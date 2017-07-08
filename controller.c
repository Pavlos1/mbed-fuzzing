#include "controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


/**
 * Connects to remote GDB server
 *
 * See: www.linuxhowtos.org/C_C++/socket.htm
 */
int gdb_connect() {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent * server;

    portno = 1234;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to open socket while communicating with GDB server\n");
        exit(1);
    }
    server = gethostbyname("localhost");
    if (!server) {
        fprintf(stderr, "Failed to resolve host while communicating with GDB server\n");
        exit(1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Failed to connect while communicating with GDB server\n");
        exit(1);
    }
    
    return sockfd;
}


/**
 * Computes checksum and sends MI command over TCP
 */
bool gdb_send_rsp_packet(int sockfd, char * command) {
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
    sprintf(payload, "$%s#%c%c\r\n", command, high_check, low_check);
    
    bool res = write(sockfd, payload, strlen(command)) >= 0;
    free(payload);
    return res;
}

/**
 * Loads debug symbols into GDB
 */
void gdb_load_symbols(char * sym_file) {
    // FIXME: This method doesn't work, for some reason
    int sockfd = gdb_connect();
    
    char * base = "symbol-file ";
    char * command = malloc(strlen(base) + strlen(sym_file) + 1);
    sprintf(command, "%s%s", base, sym_file);
    
    if (!gdb_send_rsp_packet(sockfd, command)) {
        fprintf(stderr, "WARNING: failed to load symbol file\n");
    }
    
    free(command);
    close(sockfd);
}

/**
 * Does what it says on the tin
 */
void gdb_enter_extended_mode() {
    int sockfd = gdb_connect();
    gdb_send_rsp_packet(sockfd, "!");
    close(sockfd);
}
