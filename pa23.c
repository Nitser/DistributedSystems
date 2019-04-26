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
//BalanceHistory *balance_history;

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
                	} else if(type == BALANCE_HISTORY){
				BalanceHistory balance ;
				memcpy (&balance, recieve_message.s_payload, sizeof(recieve_message.s_payload));
				//all.s_history[i-1] = balance;
				//all.s_history_len++;
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

void getDataFromMsg(void* msgData, void* outData, size_t size)
{
	for(int i=0; i<size; i++)
		((char*)outData)[i] = ((char*)msgData)[i];
}

void transfer(void * parent_data, local_id src, local_id dst, balance_t amount)
{
	char str[MAX_PAYLOAD_LEN] = "";
	int id = curPipes.id;
	TransferOrder order ;
	order.s_src = src;
	order.s_dst = dst;
	order.s_amount = amount;

	/*send TRANSFER message*/
	int w_fd = curPipes.writePipes[id][src][1];
	Message send_msg = create_message(str, sizeof order, TRANSFER);
	getDataFromMsg(&order, send_msg.s_payload, send_msg.s_header.s_payload_len);
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
		if(receive(&r_fd, dst, &recieve_message) == -1){
			sleep(1);
		} else {
			got = 1;
		}
		sleep(1);	
	}
	getDataFromMsg(recieve_message.s_payload, &order, recieve_message.s_header.s_payload_len);
	printf("Main get message from %d about %d-%d operation\n", order.s_dst, order.s_src, order.s_dst);
	//log_print(curPipes.eventsLog, events_log, str);

}

int child_start(int id){
	// printf("money = %d\n", );

	// init history
	BalanceHistory balance_history;
	balance_history.s_id = id;
	balance_history.s_history_len = 0;

	int started_count = curPipes.quantity-1;
	curPipes.id = id;
	closeUnusingPipesById(curPipes, id);
        char str[MAX_PAYLOAD_LEN] = "";
	int len = sprintf(str, log_started_fmt, id, getpid(), getppid());
	
	send_message(len, str, STARTED);
	// send_message(len, str, STARTED);
	printf("%s", str);
        log_print(curPipes.eventsLog, events_log, str);

	bool isWork = true;
	while (isWork) { 
		Message msg;
		// store_history(get_time(), pBalance[selfId]);

		if (receive_any(&curPipes, &msg) == 0) {
			switch (msg.s_header.s_type) {
			case TRANSFER: {				
					TransferOrder order;
					getDataFromMsg(msg.s_payload, &order, msg.s_header.s_payload_len);
					
					int amount = order.s_amount;

					if (order.s_src == id) {
						balance_history.s_history->s_balance -= amount;
						printf("Proess %d get TRANSFER message from %d\n", id, PARENT_ID);
						//store_history(get_time(),balance_history[id]);

						int w_fd = curPipes.writePipes[id][order.s_dst][1];
						send(&w_fd, order.s_dst, &msg);
					} else {
						balance_history.s_history->s_balance += amount;
						printf("Proess %d get TRANSFER message from %d\n", id, order.s_src);
						//store_history(get_time(), balance_history[id]);

						// msg.s_header.s_local_time = time(NULL);
						// msg.s_header.s_magic = MESSAGE_MAGIC;
						// msg.s_header.s_type = ACK;
						// msg.s_header.s_payload_len = 0;
						msg.s_header.s_type = ACK;
						int w_fd = curPipes.writePipes[id][PARENT_ID][1];
						send(&w_fd, PARENT_ID, &msg);
					}
				} break;
			case STOP: {
					isWork = false;
					printf("Process %d get STOP message\n", id);
					//store_history(get_time(), balance_history[id]);
				} break;
			case DONE: {
				printf("Process %d get DONE message\n", id);
			} break;
			case STARTED: {
				// printf("Process %d get STARTED message\n", id);
				started_count--;
				if(started_count == 0){
					sprintf(str, log_received_all_started_fmt, id);
        				printf("%s", str);
				}
			} break;
			default:
				break;
			}
			sleep(1);
		}
	}
      	//log_print(curPipes.eventsLog, events_log, str);
 
	/*len = sprintf(str, log_done_fmt, id);
	send_message(len, str, DONE);
	printf("%s", str);
        log_print(curPipes.eventsLog, events_log, str);*/

	/*recieve message*/	
	// recieve_message(len, str, DONE);	
	// sprintf(str, log_received_all_done_fmt, id);
        // printf("%s", str);
	//log_print(curPipes.eventsLog, events_log, str);


	closeUsingPipesById(curPipes, id);
        log_close(curPipes.eventsLog);

	return 0;
}

int parent_start(int id){
	  char str[MAX_PAYLOAD_LEN] = "";
          int i;
          curPipes.id = id;
	  //all.s_history_len = 0;
          closeUnusingPipesById(curPipes, id);
	  int len = sprintf(str, log_started_fmt, id, getpid(), getppid());
		
	  recieve_message(len, str, STARTED);
	  sprintf(str, log_received_all_started_fmt, id);
          printf("%s", str);
          //log_print(curPipes.eventsLog, events_log, str);

	bank_robbery(&curPipes, targetFork);
	
          /*send STOP message*/
	  len = sprintf(str, log_done_fmt, id);
          send_message(len, str, STOP);
        //   printf("%s", str);
        //   log_print(curPipes.eventsLog, events_log, str);
	
	//   /*recieve balance_history message*/
	//   recieve_message(len, str, BALANCE_HISTORY);
	//   sprintf(str, log_received_all_done_fmt, id);
        //   printf("%s", str);
        //   //log_print(curPipes.eventsLog, events_log, str);
	
  	//   //print_history(&all);	  


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
