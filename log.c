#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "log.h"

void log_print(const char * const filename, char * status, const char * const fmt, ...)
{
        va_list list;
        FILE *fp;
        va_start(list, fmt);
        fp = fopen (filename,"a+");

        if( strcmp(filename, events_log) == 0  && strcmp(status, "start") == 0) {
                fprintf(fp, fmt, va_arg(list, int), va_arg(list, int), va_arg(list, int));
        } 
//	else if( strcmp(filename, events_log) == 0 && strcmp(status, "end") == 0){
	else { 
               fprintf(fp, fmt, va_arg(list, int));
        }
        va_end(list);
        fclose(fp);
}
