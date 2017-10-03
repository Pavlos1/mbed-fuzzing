#include <stdio.h>
#include <stdlib.h>

#include "launcher.h"
#include "controller.h"
#include "util.h"
#include "elf.h"

int main() {
    char buf[1024];

    ExecStatus * stat = launch_virtual_stm32("/home/pavel/Development/mbed/qemu_stm32/arm-softmmu/qemu-system-arm",
        "/home/pavel/workspace/comp2300-lab-1/Debug/comp2300-lab-1.bin",
        "/home/pavel/workspace/comp2300-lab-1/Debug/comp2300-lab-1.elf");

    gdb_read_registers(stat);
    for (int i=0; i<N_REGS; i++) {
        printf("r%d = %x\n", i, stat->regs[i]);
    }
    
    stat->regs[0] = 0x45454545;
    gdb_write_registers(stat);
    
    if (gdb_send_rsp_packet(stat, "P1=42424242")) {
        DEBUG("single-register write OK");
    }
    
    gdb_read_registers(stat);
    for (int i=0; i<N_REGS; i++) {
        printf("r%d = %x\n", i, stat->regs[i]);
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
    
    for (int i=0; i<100; i++) {
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
}
