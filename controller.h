#ifndef ST_FUZZER_CONTROLLER
#define ST_FUZZER_CONTROLLER

#include <stdbool.h>

char * gdb_transceive_rsp_packet(int * fds, char * command);
char * gdb_read(int fd);
bool gdb_send_rsp_packet(int * fds, char * command);
void gdb_load_symbols(int * fds, char * sym_file);

#endif
