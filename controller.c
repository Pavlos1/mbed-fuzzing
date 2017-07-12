#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>

#include "controller.h"
#include "launcher.h"

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
    
    int _high_check = (checksum >> 8) & 0xf;
    if (_high_check < 10) high_check = '0' + _high_check;
    else high_check = 'a' + (_high_check - 10);
    
    char * payload = malloc(strlen(command) + 7);
    if (!payload) {
        fprintf(stderr, "Memory alloction failed\n");
        exit(1);
    }
    sprintf(payload, "$%s#%c%c\r\n", command, high_check, low_check);
    
    bool res = write(fds[PIPE_STDIN], payload, strlen(payload)) >= 0;
    free(payload);
    if (!res) return (char *) 0;
    
    return gdb_read(fds[PIPE_STDOUT]);
}


/**
 * Reads and parses as RSP packet from GDB
 *
 * TODO: Implement
 */
char * gdb_read(int fd) {
    char * ret = malloc(strlen("placeholder") + 1);
    strcpy(ret, "placeholder");
    return ret;
}


/**
 * For MI commands expecting no result
 */
bool gdb_send_rsp_packet(int * fds, char * command) {
    char * res = gdb_transceive_rsp_packet(fds, command);
    bool ret = res != (char *) 0;
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
    
    if (!gdb_send_rsp_packet(fds, command)) {
        fprintf(stderr, "WARNING: failed to load symbol file\n");
    }
    
    free(command);
}

