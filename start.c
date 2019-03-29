#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>   
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include "pa1.h"
#include "common.h"
#include "log.h"
#include "ipc.h"
#include "pipe.h"

int main( int argc, char** argv ){
	int targetFork = 1; 
	int id = 0;
	pid_t forkResult;
	ProcessPipes curPipes;

	if (argc > 2 && id == 0){	
		fclose(fopen(events_log, "w"));
		fclose(fopen(pipes_log, "w"));	
		char *p;
		errno = 0;
		if(strcmp(argv[1], "-p") == 0){
			long conv = strtol(argv[2], &p, 10);
			if (errno != 0 || *p != '\0' || conv > INT_MAX) {
				printf("Wrong data format. Use key -p X. Finish programm\n");
				return 1;
			} else {
    				targetFork = conv;    
			}	
		} else {
			printf("Wrong data format. Use key -p X. Finsh programm\n");
			return 1;
		}
	} else {
		printf("Wrong data. Use key -p X. Finish programm\n");
		return 0;
	}

	curPipes.quantity = targetFork;
	
	if(id == 0 && openPipes(&curPipes) == -1) {
		printf("Wrong event with open pipes. Finish programm\n");
		return 1;
	}
	
	do {
		forkResult = fork();
		id++;
	} while((forkResult != 0 && forkResult != -1) && (id < targetFork));

	if(forkResult == 0) {
		curPipes.id = id;

		char str[MAX_PAYLOAD_LEN] = "";
		sprintf(str, log_started_fmt, id, getpid(), getppid());
		Message send_msg = create_message(str, sizeof(char) * sizeof(str), STARTED);
		printf("Start %d process\n", curPipes.id);
		if (send_multicast(&curPipes, &send_msg) == -1) {
			return -1;
		}
		printf("%s", send_msg.s_payload);	
		log_print(events_log, send_msg.s_payload);
		
		//receive	
		int i;
		for(i=1; i<=targetFork; i++){
			Message receive_message;
			if (receive_any(&curPipes, &receive_message) == -1) {
				printf("Problem in receive message, id: %d\n", id);	
				return -1;
			}
			printf("Process %d get message: %s\n", id, receive_message.s_payload);
		}
		/*sprintf(str, log_received_all_started_fmt, id);
		log_print(events_log, str);
		printf("%s", str);
		
		/// done
		sprintf(str, log_done_fmt, id);
		Message send_msg_done = create_message(str, sizeof(char) * sizeof(str), DONE);
		if (send_multicast(&curPipes, &send_msg_done) == -1) {
			return -1;
		}
		
		log_print(events_log, send_msg_done.s_payload);
		printf("%s", str);

		Message receive_messag_done;
		if (receive_any(&curPipes, &receive_messag_done) == -1) {
			return -1;
		}

		sprintf(str, log_received_all_done_fmt, id);
		log_print(events_log, str);
		printf("%s", str);*/
		
	} else if(forkResult != -1) {
		wait(NULL);
		sleep(3);
		closeAllPipes(curPipes);
	} else {
		perror("Error while calling the fork function\n");
		return -1;
	}

	return 0;
}

