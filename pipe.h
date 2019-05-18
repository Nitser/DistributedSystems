#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>

#include <stdbool.h> 
#include "log.h"
#include "ipc.h"
#include "pa2345.h"

typedef int Pipe[15][15][2];

typedef struct {
	int id ;
	int quantity;
	Pipe writePipes;
	FILE * eventsLog;
} ProcessPipes;

enum {
	READ_ID = 0,
	WRITE_ID = 1
};

int openPipes( ProcessPipes *curPipes );

int closeUnusingPipesById( ProcessPipes curPipes, int pid );

int closeUsingPipesById( ProcessPipes curPipes, int pid);

MessageHeader create_message_header(uint16_t payload_len, MessageType type);
Message create_message(char *payload, uint16_t payload_len, MessageType type);

void increment_time();
void sync_time(timestamp_t new_time);
