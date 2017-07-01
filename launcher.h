#ifndef ST_FUZZER_LAUNCHER
#define ST_FUZZER_LAUNCHER

void launch_virtual_stm32_ex(char * qemu_executable, char * bin_file, char * sym_file);
void launch_virtual_stm32(char * qemu_executable, char * bin_file, char * elf_file);

#endif
