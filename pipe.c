#include "pipe.h"

int openPipes( ProcessPipes *curPipes ){
        int i=0;
        do {
                if (pipe(curPipes->readPipes[i]) == -1) {
                        return -1;
                }
                if (pipe(curPipes->writePipes[i]) == -1) {
                        return -1;
                }
        //      log_print(pipes_slog, "start", "Open pipe for read %d", curPipes  -> readPipes[i][0]);
                i++;
        } while((i < curPipes->quantity));
        return 0;
}

int closeAllPipes( ProcessPipes curPipes ){
        int i=0;
        do {
                if (close(curPipes.readPipes[i][0]) == -1) {
                        return -1;
                }
                if (close(curPipes.readPipes[i][1]) == -1) {
                        return -1;
                }
                if (close(curPipes.writePipes[i][0]) == -1) {
                        return -1;
                }
                if (close(curPipes.writePipes[i][1]) == -1) {
                        return -1;
                }
                i++;
        } while( i < curPipes.quantity);
        return 0;
}
