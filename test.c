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

    gdb_read_registers(stat);
    for (int i=0; i<N_REGS; i++) {
        printf("r%d = %x\n", i, stat->regs[i]);
    }
    
    stat->regs[0] = 0x4545;
    gdb_write_registers(stat);
    
    gdb_read_registers(stat);
    for (int i=0; i<N_REGS; i++) {
        printf("r%d = %x\n", i, stat->regs[i]);
    }
}
