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
#include "banking.h"

ProcessPipes curPipes;
int targetFork;
int *money_balance;

int send_message(int len, char* str, MessageType type){
	Message send_msg = create_message(str, len, type);
        if ( send_multicast(&curPipes, &send_msg) == -1) {
		return -1;
	}
	return 0;
}

void recieve_message(int len, char* str, MessageType type){
        int i;
        Message recieve_message = create_message("", len, type); 
        if(curPipes.id == 0){
		for(i=1; i<= targetFork; i++){
                	if(receive_any(&curPipes, &recieve_message) == -1){
                        	i--;
                        	sleep(1);
                	}
        	}
	} else {
 		for(i=1; i< targetFork; i++){
                        if(receive_any(&curPipes, &recieve_message) == -1){
                                i--;
                                sleep(1);
                        }
                }

	}
}

void transfer(void * parent_data, local_id src, local_id dst,
              balance_t amount)
{
	char str[MAX_PAYLOAD_LEN] = "";
	int id = curPipes.id;
	TransferOrder order;
	order.s_src = src;
	order.s_dst = dst;
	order.s_amount = amount;
	memcpy(str, &order, sizeof order);

	//printf("Transferi:\n");
	/*send TRANSFER message*/
	int w_fd = curPipes.writePipes[id][src][1];
	Message send_msg = create_message(str, sizeof order, TRANSFER);
        if ( send(&w_fd, src, &send_msg) == -1) {
               printf("error to send transfer msg\n"); 
        }
	//printf("%s", str);
	//log_print(curPipes.eventsLog, events_log, str);	
	
	/*recive result of TRANSFER message*/
	Message recieve_message = create_message(str, sizeof order, TRANSFER);
	int got = 0;
	int r_fd = curPipes.writePipes[dst][id][0];
	while( got == 0 ){
		if(receive(&r_fd, id, &recieve_message) == -1){
			sleep(1);
		} else {
			got = 1;
		}	
	}
	printf("Main get message from %d about %d-%d operation\n", dst, src, dst);
	//log_print(curPipes.eventsLog, events_log, str);

}

int child_start(int id){
	// printf("money = %d\n", money_balance[id - 1]);

	// init history
	BalanceHistory *balance_history;
	balance_history = (BalanceHistory*)malloc(sizeof(BalanceHistory));
	balance_history->s_id = id;
	balance_history->s_history_len = 0;

	curPipes.id = id;
	closeUnusingPipesById(curPipes, id);
        char str[MAX_PAYLOAD_LEN] = "";
	int len = sprintf(str, log_started_fmt, id, getpid(), getppid());
	
	send_message(len, str, STARTED);
	printf("%s", str);
        log_print(curPipes.eventsLog, events_log, str);

	/*recieve start message*/	
	recieve_message(len, str, STARTED);
        sprintf(str, log_received_all_started_fmt, id);
        printf("%s", str);
      	//log_print(curPipes.eventsLog, events_log, str);
 
	len = sprintf(str, log_done_fmt, id);
	send_message(len, str, DONE);
	printf("%s", str);
        log_print(curPipes.eventsLog, events_log, str);

	/*recieve done message*/	
	recieve_message(len, str, DONE);	
	sprintf(str, log_received_all_done_fmt, id);
        printf("%s", str);
	//log_print(curPipes.eventsLog, events_log, str);
	
	closeUsingPipesById(curPipes, id);
        log_close(curPipes.eventsLog);
	return 0;
}

int parent_start(int id){
	  AllHistory all;
	  char str[MAX_PAYLOAD_LEN] = "";
          int i;
          id = 0;
          curPipes.id = id;
          closeUnusingPipesById(curPipes, id);
	  int len = sprintf(str, log_started_fmt, id, getpid(), getppid());
		
	  recieve_message(len, str, STARTED);
	  sprintf(str, log_received_all_started_fmt, id);
          printf("%s", str);
          //log_print(curPipes.eventsLog, events_log, str);

	  bank_robbery(&curPipes, targetFork);
	 

          /*send STOP message*/
	 /* len = sprintf(str, log_done_fmt, id);
          send_message(len, str, DONE);
          printf("%s", str);
          log_print(curPipes.eventsLog, events_log, str);
	*/
	  /*recieve balance_history message*/
	  /*recieve_message(len, str, DONE);
	  sprintf(str, log_received_all_done_fmt, id);
          printf("%s", str);
          //log_print(curPipes.eventsLog, events_log, str);
	
  	  //print_history(all);	  
*/
	  for(i = 1; i<= targetFork; i++){
		  wait(NULL);
	  }
	  closeUsingPipesById(curPipes, id);
	  //log_close(curPipes.eventsLog);
	  return 0;
}

int main(int argc, char * argv[])
{
	targetFork = 1;
	int id = 0;
	int forkResult;
	if (argc > 2 ){
                char *p;
                errno = 0;
                if(strcmp(argv[1], "-p") == 0){
                        long conv = strtol(argv[2], &p, 10);
                        if (errno != 0 || *p != '\0' || conv > INT_MAX) 
			{
                                printf("Use key -p X n1 n2 n3 ...\n");
                                return 1;
                        } else {
                                targetFork = conv;
				if (argc != conv + 3) {
					printf("Use key -p X n1 n2 n3 ...\n");
					return 1;
				}
				money_balance = malloc(sizeof(int) * conv);
				for (int i = 0; i < conv; i++) {
					money_balance[i] = atoi(argv[3 + i]);
				}
                        }
                } else {
                        printf("Use key -p X n1 n2 n3 ...\n");
                        return 1;
                }
        } else {
                printf("Use key -p X n1 n2 n3 ...\n");
                return 0;
        }

        curPipes.quantity = targetFork;
	

	 do {
                if(id == 0) {
                        if (openPipes(&curPipes) == -1) {
                                printf("Wrong event with open pipes\n");
                		return 1;
                        }
        	}
                forkResult = fork();
                id++;
        } while((forkResult != 0 && forkResult != -1) && (id < targetFork));

	if(forkResult == 0) {
		child_start(id);
	} else if(forkResult != -1){
		parent_start(0);
	} else {
                perror("Error while calling the fork function\n");
                return -1;
        }
	free(money_balance);
   	return 0;
}
