#include "util.h"

/**
 * Does what it says on the tin
 *
 * Returns -1 on failure
 */
unsigned int from_hex_digit(char digit) {
	if ((digit >= 'A') && (digit <= 'F')) return digit - 'A' + 0xA;
	else if ((digit >= 'a') && (digit <= 'f')) return digit - 'a' + 0xA;
	else if ((digit >= '0') && (digit <= '9')) return digit - '0';
	else return -1;
}


void * safe_malloc(size_t size) {
    printf("[DEBUG] Allocating %d bytes\n", size);
    void * ret = malloc(size);
    if (!ret) {
        printf("[FATAL] Memory alloction failed\n");
        exit(1);
    }
    return ret;
}

bool safe_compare_null_term(char * v1, char * v2, uint32_t max_len) {
    for (int i=0; i < max_len + 1; i++) {
        if ((v1[i] == '\0') && (v2[i] == '\0')) return true;
        if (v1[i] != v2[i]) return false;
    }
    
    return false;
}
