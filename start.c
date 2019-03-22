#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>   
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "pa1.h"
#include "common.h"
#include "log.h"


int main( int argc, char** argv ){
	int targetFork = 1; 
	int id = 0;
	pid_t forkResult;
	
	if( argc > 2 && id == 0 ){	
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

	do{
		forkResult = fork();
		id++;
	} while( (forkResult != 0 && forkResult != -1) && (id < targetFork));

		if( forkResult == 0 ){
			printf(log_started_fmt, id, getpid(), getppid());
			log_print(events_log, "start", log_started_fmt, getppid(), getpid(), id); 	
			printf(log_done_fmt, id);
			log_print(events_log, "end", log_done_fmt, id);
		} else if( forkResult != -1 ){
			printf("Main is waiting...\n");
			wait(NULL);
			printf("Main is exiting\n");
		} else {
			perror("Error while calling the fork function\n");
		}

	return 0;
}

