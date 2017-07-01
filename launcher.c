#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "launcher.h"
#include "controller.h"

/**
 * Loads the binary into QEMU and starts GDB server, loading debug symbols from file given
 */
void launch_virtual_stm32_ex(char * qemu_executable, char * bin_file, char * sym_file) {
    int pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Failed to fork while trying to execute QEMU\n");
        exit(1);
    } else if (pid == 0) {
        execlp(qemu_executable, qemu_executable,
            "-kernel", bin_file,                      // ^ data initially loaded into flash
            "-machine", "stm32-p103",                 // ^ at present, the only available STM32 board
            "-S",                                     // ^ pause the VM before execution
            "-gdb", "tcp::1234" ,                     // ^ create GDB server, listen on port 1234
            (char *) 0);
    }
    
    if (sym_file) {
        load_symbols(sym_file);
    }
}

/**
 * Extracts the symbols from the ELF file given and calls `launch_virtual_stm32_ex`
 */
void launch_virtual_stm32(char * qemu_executable, char * bin_file, char * elf_file) {
    char * path = "/tmp/";
    char * ext = ".sym";
    char * sym_file = malloc(strlen(elf_file) + strlen(path) + strlen(ext) + 1);
    
    char * basename_pos = elf_file + strlen(elf_file);
    while ((basename_pos > elf_file) && (*basename_pos != '/')) basename_pos--;
    
    // sym_file = "/tmp/" ++ basename(elf_file) ++ ".sym"
    int index = 0;
    char * ptr = path;
    while (*ptr) sym_file[index++] = *(ptr++);
    ptr = basename_pos;
    while (*ptr) sym_file[index++] = *(ptr++);
    ptr = ext;
    while (*ptr) sym_file[index++] = *(ptr++);
    sym_file[index] = '\0';
    
    int pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Failed to fork while trying to execute arm-none-eabi-objcopy\n");
        exit(1);
    } else if (pid == 0) {
        exit(execlp("arm-none-eabi-objcopy", "--only-keep-debug", elf_file, sym_file, (char *) 0));
    }
    
    int status;
    if(waitpid(pid, &status, 0) < 0) {
        fprintf(stderr, "Error in waitpid() while trying to execute arm-none-eabi-objcopy\n");
        exit(1);
    }
    if (!WIFEXITED(status)) {
        fprintf(stderr, "Child process never exited (this should never happen?) while trying"
            " to execute arm-none-eabi-objcopy\n");
        exit(1);
    }
    if (WEXITSTATUS(status)) {
        fprintf(stderr, "arm-none-eabi-objcopy returned with non-zero exit status: %d\n", WEXITSTATUS(status));
        exit(1);
    }
    
    launch_virtual_stm32_ex(qemu_executable, bin_file, sym_file);
}
