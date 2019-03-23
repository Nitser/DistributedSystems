#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include "common.h"
#include "log.h"

typedef struct {
        int id;
        int quantity;
        int readPipes[15][2];
        int writePipes[15][2];
} ProcessPipes;

int openPipes( ProcessPipes *curPipes );

int closeAllPipes( ProcessPipes curPipes );
