#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>

#include "controller.h"
#include "launcher.h"


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


/**
 * Computes checksum and sends MI command
 */
char * gdb_transceive_rsp_packet(ExecStatus * stat, char * command) {
    unsigned int checksum = 0;
    for (unsigned int i=0; i<strlen(command); i++) checksum = (checksum + command[i]) & 0xff;
    
    char low_check, high_check;
    
    unsigned int _low_check = checksum & 0xf;
    if (_low_check < 10) low_check = '0' + _low_check;
    else low_check = 'a' + (_low_check - 10);
    
    unsigned int _high_check = (checksum >> 4) & 0xf;
    if (_high_check < 10) high_check = '0' + _high_check;
    else high_check = 'a' + (_high_check - 10);
    
    char * payload = malloc(strlen(command) + 6);
    if (!payload) {
        fprintf(stderr, "Memory alloction failed\n");
        exit(1);
    }
    sprintf(payload, "$%s#%c%c\n", command, high_check, low_check);
    
    printf("[DEBUG] Sending RSP packet to GDB: %s\n", payload);
    bool res = write(stat->fd_stdin, payload, strlen(payload)) >= 0;
    free(payload);
    if (!res) return NULL;
    
    return gdb_read(stat->fd_stdout);
}


/**
 * Reads and parses as RSP packet from GDB
 */
char * gdb_read(int fd) {
    // wait for transmission ACK
    char tmp;
    do {
        read(fd, &tmp, 1);
        
        if (tmp == '-') {
            printf("[DEBUG] GDB requested retransmission\n");
            return NULL;
        }
    } while (tmp != '+');
    printf("[DEBUG] Got transmission ACK\n");

    char * buf = malloc(1024);
    if (!buf) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    // wait for start of message
    do {
        read(fd, &tmp, 1);
        printf("[DEBUG] Dropping character: %c\n", tmp);
    } while (tmp != '$');
    printf("[DEBUG] Starting message read...\n");
    
    // and read the rest of the packet
    unsigned int size = read(fd, buf, 1023);
    buf[1023] = '\0';
    printf("[DEBUG] Candidate message read: %s\n", buf);
    
    if (size < 3) {
        printf("[DEBUG] RSP packet too short\n");
        printf("[DEBUG] Got response: %s\n", buf);
        free(buf);
        return NULL;
    }
    
    unsigned int checksum = 0, checksum_assert = 0;
    for (unsigned int i=0; i < size - 2; i++) {
        if (buf[i] == '#') {
            printf("[DEBUG] Found termination character\n");
            
            if (i == 0) {
                printf("[WARN] Unsupported command\n");
                free(buf);
                return NULL;
            }
            
            unsigned int msg_size = i;
            
            buf[i++] = '\0';
            checksum &= 0xff;
            
            unsigned int checksum_assert_high = from_hex_digit(buf[i++]);
            unsigned int checksum_assert_low = from_hex_digit(buf[i++]);
            if ((checksum_assert_low < 0) || (checksum_assert_high < 0)) {
                printf("[DEBUG] Checksum contains invalid characters\n");
            	free(buf);
            	return NULL;
            }
            
            checksum_assert += checksum_assert_low;
            checksum_assert += checksum_assert_high << 4;
            
            if (checksum != checksum_assert) {
                printf("[DEBUG] Checksum incorrect\n");
                printf("[DEBUG] Expecting: %d\n", checksum);
                printf("[DEBUG] Actual: %d\n", checksum_assert);
                free(buf);
                return NULL;
            }
            
            char * buf_realloc = realloc(buf, msg_size);
            if (buf_realloc != buf) {
                printf("[FATAL] Memory re-allocation failed\n");
                exit(1);
            }
            return buf_realloc;
        }
        
        checksum += buf[i];
    }
    
    printf("[DEBUG] Message never terminated\n");
    free(buf);
    return NULL;
}


/**
 * For MI commands expecting no result
 */
bool gdb_send_rsp_packet(ExecStatus * stat, char * command) {
    char * res = gdb_transceive_rsp_packet(stat, command);
    bool ret = res != NULL;
    free(res);
    
    return ret;
}


/**
 * Sets a breakpoint at requested label resumes execution
 * until breakpoint is hit. Breakpoints created this way
 * do not persist, though they may clobber already existing
 * ones.
 *
 * TODO: Implement
 */
char * gdb_ffwd_to_label(ExecStatus * stat, char * label) {
    return '\0';
}

