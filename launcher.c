#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "launcher.h"


/**
 * Loads the binary into QEMU and starts GDB server
 */
ExecStatus * launch_virtual_stm32(char * qemu_executable, char * bin_file, char * sym_file) {
    ExecData * elf_data = safe_malloc(sizeof(ExecData));
    
    if (!elf_load_symbols(elf_data, sym_file)) {
        WARN("Symbol loading failed");
    }
    
    return launch_virtual_stm32_ex(qemu_executable, bin_file, elf_data);
}


/**
 * Like launch_virtual_stm32 but uses cached ELF data
 */
ExecStatus * launch_virtual_stm32_ex(char * qemu_executable, char * bin_file, ExecData * elf_data) {
    char * argv[] = { qemu_executable,
            "-kernel", bin_file,                      // ^ data initially loaded into flash
            "-machine", "stm32-p103",                 // ^ at present, the only available STM32 board
            "-S",                                     // ^ pause the VM before execution
            "-gdb", "stdio",                          // ^ create GDB server, connect via pipe
            NULL };

    return launch_gdb_server(qemu_executable, argv, elf_data);
}


/**
 * Loads the binary onto device and starts GDB server
 */
ExecStatus * launch_physical_stm32(char * openocd_executable, char * openocd_script_dir, char * bin_file, char * sym_file) {
    ExecData * elf_data = safe_malloc(sizeof(ExecData));
    
    if (!elf_load_symbols(elf_data, sym_file)) {
        WARN("Symbol loading failed");
    }
    
    return launch_physical_stm32_ex(openocd_executable, openocd_script_dir, bin_file, elf_data);
}


/**
 * Like launch_physical_stm32 but uses cached ELF data
 */
ExecStatus * launch_physical_stm32_ex(char * openocd_executable, char * openocd_script_dir, char * bin_file, ExecData * elf_data) {
    // first flash the bin file to the board
    int pid = fork();
    
    if (pid < 0) {
        FATAL("Fork failed while trying to flash image");
        exit(1);
    } else if (pid == 0) {
        execlp("st-flash", "st-flash", "write", bin_file, "0x8000000", NULL);
        
        // if `exec` returns, something went wrong
        FATAL("Failed to exec st-flash");
        exit(1);
    }
    
    int ret_status;
    waitpid(pid, &ret_status, 0);
    
    if (ret_status) {
        FATAL("Failed to flash image");
        exit(1);
    }

    chdir(openocd_script_dir);
    char * argv[] = { openocd_executable,
            "-p",                                            // ^ connect to GDB via pipe
            "-f", "board/stm32l476g_disco.cfg",              // ^ openocd config file for our board
            NULL };
    
    return launch_gdb_server(openocd_executable, argv, elf_data);   
}


/**
 * Launches GDB server and connects via pipe
 *
 * See: https://stackoverflow.com/questions/9405985/linux-3-0-executing-child-process-with-piped-stdin-stdout
 * for pipe creation code
 */
ExecStatus * launch_gdb_server(char * exec, char ** argv, ExecData * elf_data) {
    DEBUG("entering launch_virtual_stm32_ex");

    int stdinFDs[2];
    int stdoutFDs[2];
    
    // set up pipes for communicating with GDB
    if (pipe(stdinFDs) < 0) {
        FATAL("Failed to set up pipes while launching QEMU");
        exit(1);
    }
    if (pipe(stdoutFDs) < 0) {
        close(stdinFDs[PIPE_READ]);
        close(stdoutFDs[PIPE_WRITE]);
        FATAL("Failed to set up pipes while launching QEMU");
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        FATAL("Failed to fork while trying to execute QEMU");
        exit(1);
    } else if (pid == 0) {
        // redirect stdin
        if (dup2(stdinFDs[PIPE_READ], STDIN_FILENO) == -1) {
            FATAL("Failed to set up pipes while launching QEMU");
            exit(1);
        }

        // redirect stdout
        if (dup2(stdoutFDs[PIPE_WRITE], STDOUT_FILENO) == -1) {
            FATAL("Failed to set up pipes while launching QEMU");
            exit(1);
        }
        
        // all these are for use by parent only
        close(stdinFDs[PIPE_READ]);
        close(stdinFDs[PIPE_WRITE]);
        close(stdoutFDs[PIPE_READ]);
        close(stdoutFDs[PIPE_WRITE]);
    
        execvp(exec, argv);
            
        // if `exec` returns, something went wrong
        FATAL("Failed to exec GDB server");
        exit(1);
    }
    
    // close unused file descriptors, these are for child only
    close(stdinFDs[PIPE_READ]);
    close(stdoutFDs[PIPE_WRITE]);
    
    // set up file descriptors to be returned
    ExecStatus * ret = safe_malloc(sizeof(ExecStatus));
    ret->fd_stdin  = stdinFDs[PIPE_WRITE];
    ret->fd_stdout = stdoutFDs[PIPE_READ];
    
    ret->data = elf_data;
    
    if (!enable_memory_protection(ret)) {
        WARN("Could not set up memory protection");
    }
    
    return ret;
}

