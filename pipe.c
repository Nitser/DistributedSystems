#include "pipe.h"
#include "common.h"
#include <errno.h>

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
	for (local_id id = 0; id < pipes->quantity; id++) {
		int w_fd = pipes->writePipes[id][1];
		if (w_fd != -1 && pipes->id != id) {
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
		printf("Can`t read from pipe; message_size = %zu; read_size = %zu\n", message_size, read_size);
		return -1;
	}
	return 0;
}

int receive_any(void * self, Message * msg) {
	ProcessPipes *pipes = (ProcessPipes*)self;
	for (local_id id = 0; id < pipes->quantity; id++) {
		int r_fd = pipes->writePipes[id][0];
		if (r_fd != -1 && pipes->id != id) {
			if (receive(&r_fd, id, msg) == -1) {
				return -1;
			}
		}
	}
	return 0;
}

int openPipes( ProcessPipes *curPipes ){
        int i = 0;
        do {
                if (pipe(curPipes->readPipes[i]) == -1) {
                        return -1;
                }
                if (pipe(curPipes->writePipes[i]) == -1) {
                        return -1;
		}
                i++;
        } while((i < curPipes->quantity));
	return 0;
}

int closeAllPipes( ProcessPipes curPipes ){
	int i = 0;
        do {
                if (close(curPipes.readPipes[i][0]) == -1) {
                        printf("Error closing reading end of pipe %d in %d.\n", curPipes.readPipes[i][0], i );
                        return -1;
                }
                if (close(curPipes.readPipes[i][1]) == -1) {
                        printf("Error closing writing end of pipe %d.\n", i);
                        return -1;
                }
                if (close(curPipes.writePipes[i][0]) == -1) {
                        printf("Error closing reading end of pipe %d.\n", i);
                        return -1;
                }
                if (close(curPipes.writePipes[i][1]) == -1) {
                        printf("Error closing writing end of pipe %d.\n", i);
                        return -1;
                }
                i++;
	} while( i < curPipes.quantity);
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
