#ifndef ST_FUZZER_UTIL
#define ST_FUZZER_UTIL

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

unsigned int from_hex_digit(char digit);
void * safe_malloc(size_t size);

bool safe_compare_null_term(char * v1, char * v2, uint32_t max_len);

#endif
