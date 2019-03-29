#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>

#include "log.h"
#include "ipc.h"
#include "pa1.h"

typedef int Pipe[15][15][2];

typedef struct {
	int id;
	int quantity;
	Pipe readPipes;
	Pipe writePipes;
} ProcessPipes;

enum {
	READ_ID = 0,
	WRITE_ID = 1
};

int openPipes( ProcessPipes *curPipes );

int closeAllPipes( ProcessPipes curPipes );

MessageHeader create_message_header(uint16_t payload_len, MessageType type);
Message create_message(char *payload, uint16_t payload_len, MessageType type);
