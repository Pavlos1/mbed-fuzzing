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
        
    //if (gdb_send_rsp_packet(stat, "qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;vfork-events+;exec-events+;vContSupported+;QThreadEvents+;no-resumed+")) {
    if (gdb_send_rsp_packet(stat, "qSupported:swbreak+;hwbreak+")) {
        DEBUG("qSupported OK");
    }

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
    
    Elf32_Addr main_sym = elf_lookup_symbol(&stat->data, "main");
    printf("'main' at: %d\n", main_sym);
    
    Elf32_Addr loop_sym = elf_lookup_symbol(&stat->data, "loop");
    printf("'loop' at: %d\n", loop_sym);
    
    printf("Skip to main..\n");
    if (!gdb_ffwd_to_label(stat, main_sym)) {
        FATAL("Failed to ffwd to main");
        exit(1);
    }
    printf("Got to main!\n");
    
    for (int i=0; i<100; i++) {
        printf("Skip to loop..\n");
        if (!gdb_ffwd_to_label(stat, loop_sym)) {
            FATAL("Failed to ffwd to loop");
            exit(1);
        }
        printf("Got to loop!\n");
    }
    
    printf("That's enough..\n");
}
