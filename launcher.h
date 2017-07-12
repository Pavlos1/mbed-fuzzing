#ifndef ST_FUZZER_LAUNCHER
#define ST_FUZZER_LAUNCHER

#define PIPE_READ 0
#define PIPE_WRITE 1

#define PIPE_STDIN 0
#define PIPE_STDOUT 1

int * launch_virtual_stm32_ex(char * qemu_executable, char * bin_file, char * sym_file);
int * launch_virtual_stm32(char * qemu_executable, char * bin_file, char * elf_file);

#endif
