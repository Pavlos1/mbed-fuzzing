#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "launcher.h"
#include "controller.h"
#include "util.h"
#include "elf.h"
#include "scheduler.h"

int main() {
    //start_workers(3);

    ExecStatus * stat = launch_physical_stm32(OPENOCD, "./openocd-scripts", BIN_FILE, ELF_FILE);
    //ExecStatus * stat = launch_virtual_stm32(QEMU_SYSTEM_ARM, BIN_FILE, ELF_FILE);
    
    /*char command[] = "Z4,00000000,4";
    ppr_address_32(&command[3], GPIOE_ADDRESS);
    printf("Command is: %s\n", command);
    printf("Z4 returned: %s\n", gdb_transceive_rsp_packet(stat, command));*/

    gdb_read_registers(stat);
    for (int i=0; i<N_REGS; i++) {
        printf("r%d = %x\n", i, stat->regs[i]);
    }
    
    stat->regs[0] = 0x45454545;
    gdb_write_registers(stat);
    
    if (gdb_send_rsp_packet(stat, "P1=42424242")) {
        DEBUG("single-register write OK");
    } else {
        WARN("single-register write unavailable");
    }
    
    gdb_read_registers(stat);
    for (int i=0; i<N_REGS; i++) {
        printf("r%d = %x\n", i, stat->regs[i]);
    }
    
    uint8_t test_memory[4];
    gdb_read_memory(stat, 0x20000010, 4, test_memory);
    for (int i=0; i<4; i++) {
        if (test_memory[i] == 0x45) {
            WARN("Byte %d has value 0x45. Coincidence?", i);
        } else {
            printf("Byte %d has value %d before write\n", i, test_memory[i]);
        }
    }
    
    gdb_write_word_memory_via_core(stat, 0x20000010, 0x45454545);
    gdb_read_memory(stat, 0x20000010, 4, test_memory);
    for (int i=0; i<4; i++) {
        if (test_memory[i] != 0x45) {
            WARN("Byte %d has incorrect value %d", i, test_memory[i]);
        } else {
            DEBUG("Byte %d OK", i);
        }
    }
    
    
    Elf32_Addr main_sym = elf_lookup_symbol(stat->data, "main");
    printf("'main' at: %x\n", main_sym);
    
    Elf32_Addr loop_sym = elf_lookup_symbol(stat->data, "loop");
    printf("'loop' at: %x\n", loop_sym);
    
    printf("Skip to main..\n");
    if (!gdb_ffwd_to_label(stat, main_sym)) {
        FATAL("Failed to ffwd to main");
        exit(1);
    }
    printf("Got to main!\n");
    gdb_read_registers(stat);
    printf("PC=%x\n", stat->regs[REG_PC]);
    
    for (int i=0; i<10; i++) {
        printf("Skip to loop..\n");
        if (!gdb_ffwd_to_label(stat, loop_sym)) {
            FATAL("Failed to ffwd to loop");
            exit(1);
        }
        printf("Got to loop!\n");
        gdb_read_registers(stat);
        printf("PC=%x\n", stat->regs[REG_PC]);
        gdb_send_rsp_packet(stat, "s");
    }
    
    printf("That's enough..\n");
    
    uint8_t mpu_ctrl_val[4];
    gdb_read_memory(stat, MPU_CTRL_ADDRESS, 4, mpu_ctrl_val);
    if (mpu_ctrl_val[0] & 1) {
        printf("ENABLE set\n");
    } else {
        printf("ENABLE clear\n");
    }
    
    if (mpu_ctrl_val[0] & 2) {
        printf("HFNMIENA set\n");
    } else {
        printf("HFNMIENA clear\n");
    }
    
    if (mpu_ctrl_val[0] & 4) {
        printf("PRIVDEFENA set\n");
    } else {
        printf("PRIVDEFENA clear\n");
    }
    
    pthread_exit(NULL);
}
