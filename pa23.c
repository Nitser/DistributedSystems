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
#include "pa2345.h"
#include "common.h"
#include "log.h"
#include "ipc.h"
#include "pipe.h"
#include "banking.h"

ProcessPipes curPipes;
int targetFork;

void getDataFromMsg(void* msgData, void* outData, size_t size)
{
	for(int i=0; i<size; i++)
		((char*)outData)[i] = ((char*)msgData)[i];
}

void send_message(int len, char* str, MessageType type){
	Message send_msg = create_message(str, len, type);
	while(send_multicast(&curPipes, &send_msg) == -1) {}
}

void recieve_all_messages(MessageType type){
        int i = 1;
        Message receive_message = create_message("", 0, type);
	while(i <= targetFork) {
		if(receive_any(&curPipes, &receive_message) != -1 && receive_message.s_header.s_type == type) {
			i++;
		} else {
			sleep(1);
		}
	}
}

void transfer(void * parent_data, local_id src, local_id dst, balance_t amount) {
	char str[MAX_PAYLOAD_LEN] = "";
	int id = curPipes.id;
	TransferOrder order ;
	order.s_src = src;
	order.s_dst = dst;
	order.s_amount = amount;

	increment_time();
	/*send TRANSFER message*/
	int w_fd = curPipes.writePipes[id][src][1];
	Message send_msg = create_message(str, sizeof(order), TRANSFER);
	getDataFromMsg(&order, send_msg.s_payload, send_msg.s_header.s_payload_len);
        if (send(&w_fd, src, &send_msg) == -1) {
               printf("error to send transfer msg\n"); 
	       fflush(stdout);
        }
	
	/*recive ACK of transfer*/
	Message recieve_message = create_message("", 0, ACK);
	int r_fd = curPipes.writePipes[dst][id][0];
	while(1) {
		if(receive(&r_fd, dst, &recieve_message) != -1 && recieve_message.s_header.s_type == ACK){
			break;
		} else {
			sleep(1);
		}	
	}
}

void set_new_history_state(BalanceHistory *history, timestamp_t time, balance_t amount, balance_t pending) {
	int index = -1;
	for (int idx = 0; idx < history->s_history_len; idx++) {
		if (history->s_history[idx].s_time == time) {
			index = idx;
			break;
		}
	}

	BalanceState current_state;
	
	if (index < 0 || index >= history->s_history_len) {
		if (history->s_history_len > 0) {
			current_state = history->s_history[history->s_history_len - 1];
			if (current_state.s_time < time - 1) {
				for (timestamp_t t = history->s_history[history->s_history_len - 1].s_time + 1; t < time; t++) {
					set_new_history_state(history, t, current_state.s_balance, current_state.s_balance_pending_in);
				}
			}
		}
		history->s_history[history->s_history_len] = current_state;
		history->s_history_len++;
	} else {
		history->s_history[index] = current_state;
	}
	current_state.s_balance = amount;
	current_state.s_time = time;
	current_state.s_balance_pending_in = pending;
	
}
int child_start(int id, balance_t sum){
	increment_time();
	int done_count = curPipes.quantity-1;
	int started_count = curPipes.quantity-1;
	curPipes.id = id;
	closeUnusingPipesById(curPipes, id);

        char str[MAX_PAYLOAD_LEN] = "";
	int len = sprintf(str, log_started_fmt, get_lamport_time(), id, getpid(), getppid(), sum);
	send_message(len, str, STARTED);
	printf("%s", str);
	fflush(stdout);
        log_print(curPipes.eventsLog, events_log, str);

	// init history
	BalanceHistory balance_history;
	balance_history.s_id = id;
	balance_history.s_history_len = 0;

	set_new_history_state(&balance_history, get_lamport_time(), sum, 0);

	bool hasStop = false;
	bool isWork = true;
	while (isWork) { 
		Message msg;
		if (receive_any(&curPipes, &msg) != -1) {
			switch (msg.s_header.s_type) {
			case TRANSFER: {				
				TransferOrder order;
				getDataFromMsg(msg.s_payload, &order, msg.s_header.s_payload_len);
				balance_t amount = order.s_amount;

				if (order.s_src == id) {
					sum -= amount;
					set_new_history_state(&balance_history, get_lamport_time(), sum, amount);
					set_new_history_state(&balance_history, get_lamport_time() + 2, sum, 0);

					increment_time();
					// printf("sum1 = %d \n", sum);
					int w_fd = curPipes.writePipes[id][order.s_dst][1];
					msg.s_header.s_local_time = get_lamport_time();
					send(&w_fd, order.s_dst, &msg);

					sprintf(str, log_transfer_out_fmt, get_lamport_time(), id, amount, order.s_dst);
					printf("%s", str);
					fflush(stdout);
					log_print(curPipes.eventsLog, events_log, str);
				} else {
					sprintf(str, log_transfer_in_fmt, get_lamport_time(), id, amount, order.s_src);
					printf("%s", str);
					fflush(stdout);
					log_print(curPipes.eventsLog, events_log, str);

					sum += amount;

					set_new_history_state(&balance_history, get_lamport_time(), sum, 0);

					increment_time();
					// printf("sum2 = %d \n", sum);
					msg.s_header.s_type = ACK;
					int w_fd = curPipes.writePipes[id][PARENT_ID][1];
					send(&w_fd, PARENT_ID, &msg);
				}

			} break;
			case STOP: {
				increment_time();
				hasStop = true;
				if(done_count == 0)
					isWork = false;

				sleep(1);
				len = sprintf(str, log_done_fmt, get_lamport_time(), id, sum);
				send_message(len, str, DONE);
				printf("%s", str);
				fflush(stdout);
				log_print(curPipes.eventsLog, events_log, str);

				set_new_history_state(&balance_history, get_lamport_time(), sum, 0);
			} break;
			case DONE: {
				done_count--;
				if(done_count == 0 ){
					if(hasStop)
						isWork = false;
					sprintf(str, log_received_all_done_fmt, get_lamport_time(), id);
					printf("%s", str);
					fflush(stdout);
					log_print(curPipes.eventsLog, events_log, str);
					
					increment_time();

					int w_fd = curPipes.writePipes[id][PARENT_ID][1];
					Message send_msg = create_message(str, sizeof(balance_history), BALANCE_HISTORY);
					getDataFromMsg(&balance_history, send_msg.s_payload, sizeof(balance_history));
					if (send(&w_fd, 0, &send_msg) == -1) {
						printf("error to send transfer msg\n"); 
					}
					set_new_history_state(&balance_history, get_lamport_time(), sum, 0);
				} 
			} break;
			case STARTED: {
				started_count--;
				if (started_count == 0) {
					sprintf(str, log_received_all_started_fmt, get_lamport_time(), id);
					printf("%s", str);
					fflush(stdout);
					log_print(curPipes.eventsLog, events_log, str);
				}
				
			} break;
			default:
				break;
			}
		}
		sleep(1);
	}
     
	closeUsingPipesById(curPipes, id);
        log_close(curPipes.eventsLog);

	return 0;
}

int parent_start(int id){
	int i;
	curPipes.id = id;
	closeUnusingPipesById(curPipes, id);
	increment_time();

	/*receive all started message*/
        recieve_all_messages(STARTED);

	bank_robbery(&curPipes, targetFork);

	/*send STOP message*/
	send_message(0, "", STOP);

	/*recive all done message*/
	recieve_all_messages(DONE);

	/*recieve balance_history message*/
	AllHistory *all_history = malloc(sizeof(AllHistory));
	all_history->s_history_len = targetFork;
	int complitet_balance = 1;
	while (complitet_balance <= targetFork) {
		Message msg = create_message("", sizeof(BalanceHistory), BALANCE_HISTORY);
		if (receive_any(&curPipes, &msg) != -1) {
			if (msg.s_header.s_type == BALANCE_HISTORY) {
				BalanceHistory current_history;
				getDataFromMsg(msg.s_payload, &current_history, msg.s_header.s_payload_len);
				all_history->s_history[current_history.s_id - 1] = current_history;
				fflush(stdout);
				complitet_balance++;
			}
		} else {
			sleep(1);
		}
	}

	for(i = 1; i<= targetFork; i++){
		wait(NULL);
	}
	closeUsingPipesById(curPipes, id);
	fflush(stdout);
	print_history(all_history);
	free(all_history);
	log_close(curPipes.eventsLog);
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
				fflush(stdout);
                                return 1;
                        } else {
                                targetFork = conv;
				if (argc != conv + 3) {
					printf("Use key -p X n1 n2 n3 ...\n");
					fflush(stdout);
					return 1;
				}
                        }
                } else {
                        printf("Use key -p X n1 n2 n3 ...\n");
			fflush(stdout);
                        return 1;
                }
        } else {
                printf("Use key -p X n1 n2 n3 ...\n");
		fflush(stdout);
                return 0;
        }

        curPipes.quantity = targetFork;
	

	 do {
                if(id == 0) {
                        if (openPipes(&curPipes) == -1) {
                                printf("Wrong event with open pipes\n");
				fflush(stdout);
                		return 1;
                        }
        	}
                forkResult = fork();
                id++;
        } while((forkResult != 0 && forkResult != -1) && (id < targetFork));

	if(forkResult == 0) {
		child_start(id, atoi(argv[id + 2]));
	} else if(forkResult != -1){
		parent_start(0);
	} else {
                perror("Error while calling the fork function\n");
                return -1;
        }
   	return 0;
}
