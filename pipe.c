#include "pipe.h"
#include "common.h"
#include <errno.h>
#include <fcntl.h>

int send(void * self, local_id dst, const Message * msg) {
	int w_fd = *((int*)self);
	ssize_t message_size = sizeof(Message);
	ssize_t write_size = write(w_fd, msg, message_size);
	
	if (write_size != message_size) {
		printf("Can`t write to pipe; message_size = %zu; write_size = %zu\n", message_size, write_size);
		return -1;
	}
	return 0;
}

int send_multicast(void * self, const Message * msg) {
	ProcessPipes *pipes = (ProcessPipes*)self;
	int pid = pipes->id;
	for (local_id id = 1; id <= pipes->quantity; id++) {
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
	size_t message_size = sizeof(Message);

	size_t read_size = read(r_fd, msg, message_size);
	if (read_size != message_size) {
		return -1;
	}
	return 0;
}

int receive_any(void * self, Message * msg) {
	ProcessPipes *pipes = (ProcessPipes*)self;
	int pid = pipes->id;	
	int found = -1;
	for (local_id id = 1; id <= pipes->quantity; id++) {
		int r_fd = pipes->writePipes[id][pid][0];
		if (r_fd != -1 && pid != id) {
			if (receive(&r_fd, id, msg) == -1) {
				//return -1;
			} else {
				found = 0;
				return found;
			}
		}
	}
	return found;
}

int openPipes( ProcessPipes *curPipes ){
        int i, pid;
       	for( pid = 1; pid <= curPipes->quantity; pid++){ 
		for( i = 0; i <= curPipes->quantity; i++){			
			if( i != pid ){
                		if (pipe(curPipes->writePipes[pid][i]) == -1) {
                        		printf("Error to open %d - %d pipe\n",pid, i); 	
					return -1;
				}
				if (fcntl(curPipes->writePipes[pid][i][0], F_SETFL, O_NONBLOCK) < 0){
					printf("problem in fcntl");
				}
				//printf("Open %d - %d pipe\n", pid, i);
			}
		}
	}
	return 0;
}

int closeAllPipes( ProcessPipes curPipes ){
	int i, pid ;
       	for( pid = 1; pid <= curPipes.quantity; pid++){ 
		for( i = 0; i <= curPipes.quantity; i++){
                	if (pid!= i && close(curPipes.writePipes[pid][i][0]) == -1) {
                        	printf("Error closing reading end of pipe %d in %d.\n", i, pid);
                        	return -1;
                	}
                	if (pid != i && close(curPipes.writePipes[pid][i][1]) == -1) {
                        	printf("Error closing writing end of pipe %d in %d.\n", i, pid);
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
	header.s_local_time  = time(NULL);
	return header;
}

Message create_message(char *payload, uint16_t payload_len, MessageType type) {
	Message msg;
	msg.s_header = create_message_header(payload_len, type);
	strcpy(msg.s_payload, payload);
 	return msg;
}
