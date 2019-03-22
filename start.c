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
#include "ipc.h"

typedef struct {
	int id;
	int quantity;
	int readPipes[15][2];
	int writePipes[15][2];
} ProcessPipes;

int openPipes( ProcessPipes *curPipes ){
        int i=0;
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
	int i=0;
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

int main( int argc, char** argv ){
	int targetFork = 1; 
	int id = 0;
	pid_t forkResult;
	ProcessPipes curPipes;

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

	curPipes.quantity = targetFork;
	
	if( id == 0 && openPipes(&curPipes) == -1 ) {
		printf("Wrong event with open pipes. Finish programm\n");
		return 1;
	}
	if( id == 0 && closeAllPipes(curPipes) == -1 ) {
		printf("Wrong event with close pipes. Finish programm.\n");
		return 1;
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

