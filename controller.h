#ifndef ST_FUZZER_CONTROLLER
#define ST_FUZZER_CONTROLLER

#include <stdbool.h>
#include <stdint.h>


#define REG_FP 12
#define REG_SP 13
#define REG_LR 14
#define REG_PC 15
#define REG_xPSR 16

#define N_REGS 17

typedef struct {
    int fd_stdin;
    int fd_stdout;
    
    // 16 general (rN) regs + xPSR
    uint32_t regs[17];
    // bitfield
    uint64_t regs_avail;
    
} ExecStatus;

#include "util.h"
#include "launcher.h"

char * gdb_transceive_rsp_packet(ExecStatus * stat, char * command);
char * gdb_read(ExecStatus * stat);
bool gdb_send_rsp_packet(ExecStatus * stat, char * command);
char * gdb_ffwd_to_label(ExecStatus * stat, char * label);

void gdb_read_registers(ExecStatus * stat);
void gdb_write_registers(ExecStatus * stat);

#endif
