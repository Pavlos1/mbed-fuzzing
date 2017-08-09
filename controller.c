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
int from_hex_digit(char digit) {
	if ((digit >= 'A') && (digit <= 'F')) return digit - 'A' + 0xA;
	else if ((digit >= 'a') && (digit <= 'f')) return digit - 'a' + 0xA;
	else if ((digit >= '0') && (digit <= '9')) return digit - '0';
	else return -1;
}


/**
 * Computes checksum and sends MI command
 */
char * gdb_transceive_rsp_packet(int * fds, char * command) {
    int checksum = 0;
    for (int i=0; i<strlen(command); i++) checksum = (checksum + command[i]) & 0xff;
    
    char low_check, high_check;
    
    int _low_check = checksum & 0xf;
    if (_low_check < 10) low_check = '0' + _low_check;
    else low_check = 'a' + (_low_check - 10);
    
    int _high_check = (checksum >> 4) & 0xf;
    if (_high_check < 10) high_check = '0' + _high_check;
    else high_check = 'a' + (_high_check - 10);
    
    char * payload = malloc(strlen(command) + 6);
    if (!payload) {
        fprintf(stderr, "Memory alloction failed\n");
        exit(1);
    }
    sprintf(payload, "$%s#%c%c\n", command, high_check, low_check);
    
    printf("[DEBUG] Sending RSP packet to GDB: %s\n", payload);
    bool res = write(fds[PIPE_STDIN], payload, strlen(payload)) >= 0;
    free(payload);
    if (!res) return NULL;
    
    return gdb_read(fds[PIPE_STDOUT]);
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
    do read(fd, &tmp, 1); while (tmp != '$');
    printf("[DEBUG] Starting message read...\n");
    
    // and read the rest of the packet
    int size = read(fd, buf, 1024);
    printf("[DEBUG] Candidate message read\n");
    
    if (size < 4) {
        printf("[DEBUG] RSP packet too short\n");
        printf("[DEBUG] Got response: %s\n", buf);
        free(buf);
        return NULL;
    }
    
    int checksum = 0, checksum_assert = 0, i = 2;
    bool ok = false;
    for (int i=0; i < size - 2; i++) {
        if (i == '#') {
            ok = true;
            buf[i++] = '\0';
            
            int checksum_assert_high = from_hex_digit(buf[i++]);
            int checksum_assert_low = from_hex_digit(buf[i++]);
            if ((checksum_assert_low < 0) || (checksum_assert_high < 0)) {
                printf("[DEBUG] Checksum contains invalid characters\n");
            	free(buf);
            	return NULL;
            }
            
            checksum_assert += checksum_assert_low;
            checksum_assert += checksum_assert_high << 4;
            
            if (checksum != (checksum_assert & 0xff)) {
                printf("[DEBUG] Checksum incorrect\n");
                free(buf);
                return NULL;
            }
            
            return buf;
        }
        
        checksum += buf[i];
    }
    
    return NULL;
}


/**
 * For MI commands expecting no result
 */
bool gdb_send_rsp_packet(int * fds, char * command) {
    char * res = gdb_transceive_rsp_packet(fds, command);
    bool ret = res != NULL;
    free(res);
    
    return ret;
}


/**
 * Loads debug symbols into GDB
 */
void gdb_load_symbols(int * fds, char * sym_file) {
    char * base = "symbol-file ";
    char * command = malloc(strlen(base) + strlen(sym_file) + 1);
    if (!command) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    sprintf(command, "%s%s", base, sym_file);
    
    printf(gdb_transceive_rsp_packet(fds, command));
    
    /*
    if (!gdb_send_rsp_packet(fds, command)) {
        fprintf(stderr, "WARNING: failed to load symbol file\n");
    }
    */
    
    free(command);
}

