#define _GNU_SOURCE

#include "pipe.h"
#include "common.h"
#include "banking.h"
#include <errno.h>
#include <fcntl.h>

timestamp_t current_time = 0;

timestamp_t get_lamport_time() {
	return current_time;
}

void increment_time() {
	current_time++;
}

void sync_time(timestamp_t new_time) {
	current_time = new_time > current_time ? new_time : current_time;
	current_time++;
}

int send(void * self, local_id dst, const Message * msg) {
	int w_fd = *((int*)self);
	ssize_t message_size = sizeof(MessageHeader) + (*msg).s_header.s_payload_len;
	ssize_t write_size = write(w_fd, msg, message_size);
	
	if (write_size != message_size) {
		printf("Can`t write to pipe; message_size = %zu; write_size = %zu\n", message_size, write_size);
		fflush(stdout);
		return -1;
	}
	return 0;
}

int send_multicast(void * self, const Message * msg) {
	ProcessPipes *pipes = (ProcessPipes*)self;
	int pid = pipes->id;
	sleep(1);
	for (local_id id = 0; id <= pipes->quantity; id++) {
		int w_fd = pipes->writePipes[pid][id][1];
		if (w_fd != -1 && pid != id) {
			if (send(&w_fd, id, msg) == -1) {
				return -1;
			} 
		}
	}
	return 0;
}

int receive(void * self, local_id from, Message * msg) {
	int r_fd = *((int*)self);
	int header_size = read(r_fd, &(msg->s_header), sizeof(MessageHeader));
	if (header_size > 0) {
		if ((*msg).s_header.s_payload_len > 0) {
			int payload_size = read(r_fd, msg->s_payload, (*msg).s_header.s_payload_len);
			if (payload_size < 0) {
				return -1;
			}
		}
		sync_time(msg->s_header.s_local_time);
		return 0;
	}
	return -1;	
}

int receive_any(void * self, Message * msg) {
	ProcessPipes *pipes = (ProcessPipes*)self;
	int pid = 0;
	pid = pipes->id;
	int found = -1;
	for (local_id id = 0; id <= pipes->quantity; id++) {
		int r_fd = pipes->writePipes[id][pid][0];
		if (r_fd != -1 && pid != id) {
			if (receive(&r_fd, id, msg) == -1) {
			} else {
				sync_time(msg->s_header.s_local_time);
				found = 0;
				return found;
			}
		}
	}
	return found;
}

int openPipes( ProcessPipes *curPipes ){
	int i, pid;
	char str[MAX_PAYLOAD_LEN] = "";
	FILE * fpipes_log = log_open(pipes_log);
	curPipes->eventsLog = log_open(events_log);	
       	for(pid = 0; pid <= curPipes->quantity; pid++){ 
		for(i = 0; i <= curPipes->quantity; i++){			
			if(i != pid ){
				if (pipe2(curPipes->writePipes[pid][i], O_NONBLOCK) == -1) {
					printf("Error to open %d - %d pipe\n",pid, i);
					fflush(stdout);
					return -1;
				}
				sprintf(str, "Open %d - %d pipe\n", pid, i); 
				log_print(fpipes_log, pipes_log, str);
			}
		}
	}
	log_close(fpipes_log);
	return 0;
}

int closeUnusingPipesById( ProcessPipes curPipes, int pid){
	int i, j;
	for( i=0; i<=curPipes.quantity; i++){
		for( j=0; j<=curPipes.quantity; j++ ){	
			if ( i != pid && j != pid && i != j ) {
				if (close(curPipes.writePipes[i][j][0]) == -1) {
					printf("Error closing reading end of pipe %d in %d.\n", i, j);
					fflush(stdout);
					return -1;
				}
				if (close(curPipes.writePipes[i][j][1]) == -1) {
					printf("Error closing writing end of pipe %d in %d.\n", i, j);
					fflush(stdout);
					return -1;
				}
			} else if (i != j && i == pid ){
				if (close(curPipes.writePipes[i][j][0]) == -1) {
					printf("Error closing reading end of pipe %d in %d.\n", i, j);
					fflush(stdout);
					return -1;
				}
			} else if ( i!=j && j == pid ) {
				 if (close(curPipes.writePipes[i][j][1]) == -1) {
					printf("Error closing writing of pipe %d in %d.\n", i, j);
					fflush(stdout);
					return -1;
				}
			}
		}	
	}	
	return 0;
}

int closeUsingPipesById( ProcessPipes curPipes, int pid ){
	int i;
       	for( i = 0; i <= curPipes.quantity; i++){ 
			if(pid != i){
				if (close(curPipes.writePipes[pid][i][1]) == -1) {
					printf("Error closing writing end of pipe %d in %d.\n", i, pid);
					return -1;
				}
				if (close(curPipes.writePipes[i][pid][0]) == -1) {
					printf("Error closing reading end of pipe %d in %d.\n", i, pid);
					return -1;
				}
			}
	}
	return 0;
}

MessageHeader create_message_header(uint16_t payload_len, MessageType type) {
	MessageHeader header;
	header.s_magic= MESSAGE_MAGIC;
	header.s_payload_len = payload_len;
	header.s_type = type;
	header.s_local_time  = get_lamport_time();
	return header;
}

MessageHeader create_empty_message_header(){
	MessageHeader header;
	header.s_magic = MESSAGE_MAGIC;
	header.s_local_time = get_lamport_time();
	return header;
}

Message create_message(char *payload, uint16_t payload_len, MessageType type) {
	Message msg;
	msg.s_header = create_message_header(payload_len, type);
	memcpy(msg.s_payload, &payload, payload_len);
 	return msg;
}
