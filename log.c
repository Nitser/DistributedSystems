#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "log.h"

void log_print(const char * const filename, const char * const fmt)
{
        FILE *fp;
        fp = fopen (filename,"a+");
        fprintf(fp, "%s", fmt);
        fclose(fp);
}
