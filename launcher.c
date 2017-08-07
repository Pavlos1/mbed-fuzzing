#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "launcher.h"
#include "controller.h"


/**
 * Loads the binary into QEMU and starts GDB server, loading debug symbols from file given
 *
 * See: https://stackoverflow.com/questions/9405985/linux-3-0-executing-child-process-with-piped-stdin-stdout
 * for pipe creation code
 */
int * launch_virtual_stm32_ex(char * qemu_executable, char * bin_file, char * sym_file) {
    int stdinFDs[2];
    int stdoutFDs[2];
    
    // set up pipes for communicating with GDB
    if (pipe(stdinFDs) < 0) {
        fprintf(stderr, "Failed to set up pipes while launching QEMU\n");
        exit(1);
    }
    if (pipe(stdoutFDs) < 0) {
        close(stdinFDs[PIPE_READ]);
        close(stdoutFDs[PIPE_WRITE]);
        fprintf(stderr, "Failed to set up pipes while launching QEMU\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Failed to fork while trying to execute QEMU\n");
        exit(1);
    } else if (pid == 0) {
        // redirect stdin
        if (dup2(stdinFDs[PIPE_READ], STDIN_FILENO) == -1) {
            fprintf(stderr, "Failed to set up pipes while launching QEMU\n");
            exit(1);
        }

        // redirect stdout
        if (dup2(stdoutFDs[PIPE_WRITE], STDOUT_FILENO) == -1) {
            fprintf(stderr, "Failed to set up pipes while launching QEMU\n");
            exit(1);
        }
        
        // all these are for use by parent only
        close(stdinFDs[PIPE_READ]);
        close(stdinFDs[PIPE_WRITE]);
        close(stdoutFDs[PIPE_READ]);
        close(stdoutFDs[PIPE_WRITE]);
    
        execlp(qemu_executable, qemu_executable,
            "-kernel", bin_file,                      // ^ data initially loaded into flash
            "-machine", "stm32-p103",                 // ^ at present, the only available STM32 board
            "-S",                                     // ^ pause the VM before execution
            "-gdb", "stdio",                          // ^ create GDB server, connect via pipe
            (char *) 0);
            
        // if `exec` returns, something went wrong
        fprintf(stderr, "Failed to exec QEMU\n");
        exit(1);
    }
    
    // close unused file descriptors, these are for child only
    close(stdinFDs[PIPE_READ]);
    close(stdoutFDs[PIPE_WRITE]);
    
    // set up file descriptors to be returned
    int * ret = malloc(2 * sizeof(int));
    if (!ret) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(1);
    }
    ret[PIPE_STDIN]  = stdinFDs[PIPE_WRITE];
    ret[PIPE_STDOUT] = stdoutFDs[PIPE_READ];
    
    // load symbols from file
    if (sym_file) {
        gdb_load_symbols(ret, sym_file);
    }
    
    // return file descriptors
    return ret;
}


/**
 * Extracts the symbols from the ELF file given and calls `launch_virtual_stm32_ex`
 */
int * launch_virtual_stm32(char * qemu_executable, char * bin_file, char * elf_file) {
    char * path = "/tmp/";
    char * ext = ".sym";
    char * sym_file = malloc(strlen(elf_file) + strlen(path) + strlen(ext) + 1);
    if (!sym_file) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    char * basename_pos = elf_file + strlen(elf_file);
    while ((basename_pos > elf_file) && (*basename_pos != '/')) basename_pos--;
    
    sprintf(sym_file, "%s%s%s", path, basename_pos, ext);
    
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
    
    int * ret = launch_virtual_stm32_ex(qemu_executable, bin_file, sym_file);
    free(sym_file);
    return ret;
}

