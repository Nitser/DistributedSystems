#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include "ipc.h"
#include "pipe.h"

int send_multicast( void * self, const Message * msg){
	ProcessPipes *curPipes = self;
	if( msg -> s_header.s_type == STARTED ){
		int i;
		for( i=0; i <= curPipes -> quantity; i++){
			/*if(i == curPipes->id){
				write(curPipes->writePipes[i][1], msg->s_payload, strlen(msg->s_payload)+1);
			} else*/ 
				write(curPipes->writePipes[i][0], msg->s_payload, strlen(msg->s_payload)+1);
			
		}
	} else if( msg -> s_header.s_type == DONE ){
		int i;
                for( i=0; i <= curPipes -> quantity; i++){
                        /*if(i == curPipes->id){
                                write(curPipes->writePipes[i][1], msg->s_payload, strlen(msg->s_payload)+1);

                                if (close(curPipes->writePipes[i][1]) == -1) {
                                        printf("Error closing writing end of pipe 1 in own after writing.\n");
                                        return -1;
                                }
                        } else*/ 
                                write(curPipes->writePipes[i][0], msg->s_payload, strlen(msg->s_payload)+1);

                                if (close(curPipes->writePipes[i][0]) == -1) {
                                        printf("Error closing writing end of pipe 0 in anth after writing.\n");
                                        return -1;
                                
                        }
                }		
	}
	return 0;
}


int receive_any(void * self, Message * msg){
	ProcessPipes *curPipes = self;
	const int BSIZE = 100;
	char buf[BSIZE];

        if( msg -> s_header.s_type == STARTED ){
                int i;
                for( i=0; i <= curPipes->quantity; i++){
                        if(i == curPipes->id){
                                read(curPipes->readPipes[i][0], buf, BSIZE);
				printf("Get message to %d: %s", curPipes->id, buf);				

                        }/* else {
				read(curPipes->writePipes[i][0], buf, BSIZE);
				printf("Get message to %d: %s", curPipes->id, buf);
                        }*/
                }
        } else if( msg -> s_header.s_type == DONE ){
                int i;
                for( i=0; i <= curPipes -> quantity; i++){
                        if(i == curPipes->id){
				read(curPipes->readPipes[i][0], buf, BSIZE);
                                if (close(curPipes->readPipes[i][0]) == -1) {
                                        printf("Error closing writing end of pipe 1 in own after writing.\n");
                                        return -1;
                                }
                        } /*else {
				read(curPipes->writePipes[i][0], buf, BSIZE);
                                if (close(curPipes->writePipes[i][0]) == -1) {
                                        printf("Error closing writing end of pipe 0 in anth after writing.\n");
                                        return -1;
                                }
                        }*/
                }
        }
        return 0;
}
