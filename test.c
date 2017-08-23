#include <stdio.h>
#include <stdlib.h>

#include "launcher.h"
#include "controller.h"
#include "util.h"

int main() {
    char buf[1024];

    ExecStatus * stat = launch_virtual_stm32("/home/pavel/Development/mbed/qemu_stm32/arm-softmmu/qemu-system-arm",
        "/home/pavel/workspace/comp2300-lab-1/Debug/comp2300-lab-1.bin",
        "/home/pavel/workspace/comp2300-lab-1/Debug/comp2300-lab-1.elf");
        
    if (gdb_send_rsp_packet(stat, "qSupported:multiprocess+;swbreak+;hwbreak+;qRelocInsn+;fork-events+;vfork-events+;exec-events+;vContSupported+;QThreadEvents+;no-resumed+")) {
        printf("[DEBUG] qSupported OK\n");
    }

    gdb_read_registers(stat);
    for (int i=0; i<N_REGS; i++) {
        printf("r%d = %x\n", i, stat->regs[i]);
    }
    
    stat->regs[0] = 0x45454545;
    gdb_write_registers(stat);
    
    if (gdb_send_rsp_packet(stat, "P1=42424242")) {
        printf("[DEBUG] single-register write OK\n");
    }
    
    gdb_read_registers(stat);
    for (int i=0; i<N_REGS; i++) {
        printf("r%d = %x\n", i, stat->regs[i]);
    }
}
