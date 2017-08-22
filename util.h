#ifndef ST_FUZZER_UTIL
#define ST_FUZZER_UTIL

#include <stdlib.h>
#include <stdio.h>

unsigned int from_hex_digit(char digit);
void * safe_malloc(size_t size);

#endif
