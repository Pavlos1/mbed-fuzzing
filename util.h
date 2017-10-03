#ifndef ST_FUZZER_UTIL
#define ST_FUZZER_UTIL

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


#ifdef LOG_DEBUG
    #define DEBUG(format, ...) printf("[DEBUG] " format "\n", ##__VA_ARGS__);
#else
    #define DEBUG(...)
#endif

#ifdef LOG_WARN
    #define WARN(format, ...) printf("[WARN] " format "\n", ##__VA_ARGS__);
#else
    #define WARN(...)
#endif

#ifdef LOG_FATAL
    #define FATAL(format, ...) printf("[FATAL] " format "\n", ##__VA_ARGS__);
#else
    #define FATAL(...)
#endif


#define QEMU_SYSTEM_ARM "/home/pavel/Development/mbed/qemu_stm32/arm-softmmu/qemu-system-arm"
#define BIN_FILE "/home/pavel/workspace/comp2300-lab-1/Debug/comp2300-lab-1.bin"
#define ELF_FILE "/home/pavel/workspace/comp2300-lab-1/Debug/comp2300-lab-1.elf"


/**
 * Does what it says on the tin
 *
 * Returns -1 on failure
 */
static inline unsigned int from_hex_digit(char digit) {
	if ((digit >= 'A') && (digit <= 'F')) return digit - 'A' + 0xA;
	else if ((digit >= 'a') && (digit <= 'f')) return digit - 'a' + 0xA;
	else if ((digit >= '0') && (digit <= '9')) return digit - '0';
	else return -1;
}



/*
 * Prints a 32-bit address in hex
 */
static inline void ppr_address_32(char * buf, unsigned int addr) {
    for (unsigned int i=0; i<8; i++) {
        unsigned int cur = (addr & (0xf << (i << 2))) >> (i << 2);
        buf[7-i] = cur >= 10 ? cur - 10 + 'A' : cur + '0';
    }
}


static inline void * safe_malloc(size_t size) {
    DEBUG("Allocating %d bytes", size);
    void * ret = malloc(size);
    if (!ret) {
        FATAL("Memory alloction failed");
        exit(1);
    }
    return ret;
}

bool safe_compare_null_term(char * v1, char * v2, uint32_t max_len);

#endif
