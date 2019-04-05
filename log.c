#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "log.h"

/*void log_print(const char * const filename, const char * const fmt)
{
        FILE *fp;
        fp = fopen (filename,"a+");
       	fprintf(fp, "%s", fmt);

        
	fclose(fp);
}*/


FILE * log_open(const char * const filename)
{
	FILE *fp;
	fclose(fopen(filename, "w"));
	fp = fopen (filename, "a+");
	return fp;
}

void log_print(FILE * fp, const char * const filename, const char * const fmt) 
{
	fprintf(fp, "%s", fmt);
}

void log_close( FILE * fp ){
	fclose(fp);
}
