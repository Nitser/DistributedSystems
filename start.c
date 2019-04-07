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
	int forkResult;

	ProcessPipes curPipes;

	if (argc > 2 ){	
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
	
	do {
		if(id == 0) {
			if (openPipes(&curPipes) == -1) {
				printf("Wrong event with open pipes. Finish programm\n");
            	return 1;
			}
        }
		forkResult = fork();
		id++;
	} while((forkResult != 0 && forkResult != -1) && (id < targetFork));

	if(forkResult == 0) {
		curPipes.id = id;
		closeUnusingPipesById(curPipes, id);
		char str[MAX_PAYLOAD_LEN] = "";
		int len = sprintf(str, log_started_fmt, id, getpid(), getppid());
		
		Message send_msg = create_message(str, len, STARTED);
		if (send_multicast(&curPipes, &send_msg) == -1) {
			return -1;
		}
		printf("%s", send_msg.s_payload);	
		log_print(curPipes.eventsLog, events_log, send_msg.s_payload);
		
		//receive	
		int i;	
		Message receive_message = create_message("", len, STARTED);
		for(i = 1; i < targetFork; i++){
			if (receive_any(&curPipes, &receive_message) == -1) {
				i--;
				sleep(1);
			}
		}
		sprintf(str, log_received_all_started_fmt, id);
		printf("%s", str);	
		log_print(curPipes.eventsLog, events_log, str);
	
		/// done
		len = sprintf(str, log_done_fmt, id);
		Message send_msg_done = create_message(str, len, DONE);
		if (send_multicast(&curPipes, &send_msg_done) == -1) {
			return -1;
		}
		printf("%s", str);	
		log_print(curPipes.eventsLog, events_log, send_msg_done.s_payload);
	
		// recieve done
		Message receive_message_done = create_message("", len, DONE);
		for(i = 1; i < targetFork; i++){
			if (receive_any(&curPipes, &receive_message_done) == -1) {
				i--;
				sleep(1);
			}
		}
		sprintf(str, log_received_all_done_fmt, id);
		printf("%s", str);	
		log_print(curPipes.eventsLog, events_log, str);
		closeUsingPipesById(curPipes, id);
		log_close(curPipes.eventsLog);
		
	} else if(forkResult != -1) {
		char str[MAX_PAYLOAD_LEN] = "";	
		int i;
		id = 0;
		curPipes.id = id;
		closeUnusingPipesById(curPipes, id);	

		//recieve start messages
		int len = sprintf(str, log_started_fmt, id, getpid(), getppid());
                Message receive_message = create_message("", len, STARTED);	
		for(i = 1; i <= targetFork; i++){
			if (receive_any(&curPipes, &receive_message) == -1) {	
					i--;	
					sleep(1);
			}
		}
		sprintf(str, log_received_all_started_fmt, id);
		printf("%s", str);
		//log_print(curPipes.eventsLog, events_log, str);	
	
		//recieve done messages	
		len = sprintf(str, log_done_fmt, id);	
		receive_message = create_message("", len, DONE);	
		for(i=1; i<= targetFork; i++){
			if (receive_any(&curPipes, &receive_message) == -1) {
					i--;
					sleep(1);
			} 
		}
                sprintf(str, log_received_all_done_fmt, id);
		//log_print(curPipes.eventsLog, events_log, str);	
                printf("%s", str);

		for(i = 1; i<= targetFork; i++){	
			wait(NULL);
		}
		closeUsingPipesById(curPipes, id);
		//log_close(curPipes.eventsLog);
	} else {
		perror("Error while calling the fork function\n");
		return -1;
	}

	return 0;
}

