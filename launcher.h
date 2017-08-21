#ifndef ST_FUZZER_LAUNCHER
#define ST_FUZZER_LAUNCHER

#define PIPE_READ 0
#define PIPE_WRITE 1

#include "util.h"
#include "controller.h"

ExecStatus * launch_virtual_stm32(char * qemu_executable, char * bin_file, char * sym_file);

#endif
