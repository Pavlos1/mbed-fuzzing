#include "controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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
 * Loads debug symbols into GDB
 */
void load_symbols(char * sym_file) {
    // FIXME: This method doesn't work, for some reason
    int sockfd = gdb_connect();
    
    char * base = "symbol-file ";
    char * payload = malloc(strlen(base) + strlen(sym_file) + 1);
    sprintf(payload, "%s%s", base, sym_file);
    
    int checksum = 0;
    for (int i=0; i<strlen(payload); i++) checksum = (checksum + payload[i]) & 0xff;
    
    char low_check, high_check;
    
    int _low_check = checksum & 0xf;
    if (_low_check < 10) low_check = '0' + _low_check;
    else low_check = 'a' + (_low_check - 10);
    
    int _high_check = (checksum >> 8) & 0xf;
    if (_high_check < 10) high_check = '0' + _high_check;
    else high_check = 'a' + (_high_check - 10);
    
    char * command = malloc(strlen(payload) + 7);
    sprintf(command, "$%s#%c%c\r\n", payload, high_check, low_check);
    
    if (write(sockfd, command, strlen(command)) < 0) {
        fprintf(stderr, "WARNING: failed to load symbol file\n");
    }
    
    close(sockfd);
}
