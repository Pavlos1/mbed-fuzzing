#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "launcher.h"


/**
 * Loads the binary into QEMU and starts GDB server, loading debug symbols from file given
 *
 * See: https://stackoverflow.com/questions/9405985/linux-3-0-executing-child-process-with-piped-stdin-stdout
 * for pipe creation code
 */
ExecStatus * launch_virtual_stm32(char * qemu_executable, char * bin_file, char * sym_file) {
    printf("[DEBUG] entering launch_virtual_stm32_ex\n");

    int stdinFDs[2];
    int stdoutFDs[2];
    
    // set up pipes for communicating with GDB
    if (pipe(stdinFDs) < 0) {
        fprintf(stderr, "[FATAL] Failed to set up pipes while launching QEMU\n");
        exit(1);
    }
    if (pipe(stdoutFDs) < 0) {
        close(stdinFDs[PIPE_READ]);
        close(stdoutFDs[PIPE_WRITE]);
        fprintf(stderr, "[FATAL] Failed to set up pipes while launching QEMU\n");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        fprintf(stderr, "[FATAL] Failed to fork while trying to execute QEMU\n");
        exit(1);
    } else if (pid == 0) {
        // redirect stdin
        if (dup2(stdinFDs[PIPE_READ], STDIN_FILENO) == -1) {
            fprintf(stderr, "[FATAL] Failed to set up pipes while launching QEMU\n");
            exit(1);
        }

        // redirect stdout
        if (dup2(stdoutFDs[PIPE_WRITE], STDOUT_FILENO) == -1) {
            fprintf(stderr, "[FATAL] Failed to set up pipes while launching QEMU\n");
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
            NULL);
            
        // if `exec` returns, something went wrong
        fprintf(stderr, "[FATAL] Failed to exec QEMU\n");
        exit(1);
    }
    
    // close unused file descriptors, these are for child only
    close(stdinFDs[PIPE_READ]);
    close(stdoutFDs[PIPE_WRITE]);
    
    // set up file descriptors to be returned
    ExecStatus * ret = safe_malloc(sizeof(ExecStatus));
    ret->fd_stdin  = stdinFDs[PIPE_WRITE];
    ret->fd_stdout = stdoutFDs[PIPE_READ];
    
    printf("[DEBUG] loading symbol files...\n");
    
    if (!elf_load_symbols(&ret->data, sym_file)) {
        printf("[WARN] Symbol loading failed\n");
    }
    
    // return file descriptors
    return ret;
}

