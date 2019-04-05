#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"

FILE * log_open(const char * const filename);

void log_print(FILE * fp, const char * const filename, const char * const fmt);

void log_close( FILE * fp );
