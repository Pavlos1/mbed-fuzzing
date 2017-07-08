#ifndef ST_FUZZER_CONTROLLER
#define ST_FUZZER_CONTROLLER

#include <stdbool.h>

int  gdb_connect();
bool gdb_send_rsp_packet(int sockfd, char * command);
void gdb_load_symbols(char * sym_file);
void gdb_enter_extended_mode();

#endif
