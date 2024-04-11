#include <stdio.h>
#include <errno.h>

// well I didn't want to have to do it but ugh

FILE* stdin = (FILE*)0;
FILE* stdout = (FILE*)1;
FILE* stderr = (FILE*)2;

error_t errno = 0;

