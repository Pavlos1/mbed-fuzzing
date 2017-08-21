#ifndef ST_FUZZER_CONTROLLER
#define ST_FUZZER_CONTROLLER

#include <stdbool.h>

typedef struct {
    int fd_stdin;
    int fd_stdout;
    
} ExecStatus;

char * gdb_transceive_rsp_packet(ExecStatus * stat, char * command);
char * gdb_read(int fd);
bool gdb_send_rsp_packet(ExecStatus * stat, char * command);
char * gdb_ffwd_to_label(ExecStatus * stat, char * label);

#endif
