#include "memprotect.h"


/**
 * Edits the PMSAv7 registers to enable memory protection
 * See: ARMv7-M Architecture Reference Manual, B3.5
 *
 * Also sets a breakpoint on the destination of the
 * MemManage interrupt, to trap memory faults
 *
 * TODO: Also apparently the remote stub can write memory
 *       however it wants when using 'M', which is a problem
 *       because the MPU registers must be word-accessed
 */
bool enable_memory_protection(ExecStatus * stat) {
    // check whether MPU is available
    uint8_t mpu_type_val[4];
    gdb_read_memory(stat, MPU_TYPE_ADDRESS, 4, mpu_type_val);
    if (!mpu_type_val[1]) {
        // no protected regions available..
        WARN("MPU not present");
        return false;
    }

    // get ISR entry for MemManage
    uint8_t _handler[4];
    gdb_read_memory(stat, MEM_MANAGE_ISR_ADDRESS, 4, _handler);
    uint32_t handler = _handler[0] | (_handler[1] << 8)
        | (_handler[2] << 16) | (_handler[3] << 24);
    
    // put a breakpoint on it
    char trap_memfault_command[] = "Z1,00000000,2";
    ppr_address_32(&trap_memfault_command[3], handler);
    if (!gdb_transceive_rsp_packet(stat, trap_memfault_command)) {
        WARN("Could not set up memory exception trap");
        return false;
    }
    
    // encode default memory region, for testing purposes this shall
    // be the entire GPIOE block
    // NOTE: here we using the RBAR register to also set the region number
    uint32_t _mpu_rbar_val = ((GPIOE_ADDRESS >> 5) << 5) // clear last 5 bits for other fields
        | (1 << 4)                                       // set VALID => lowest 4 bits are memory region
            | 1;                                         // memory region itself
    uint8_t mpu_rbar_val[4];
    mpu_rbar_val[0] = _mpu_rbar_val & 0xff;
    mpu_rbar_val[1] = (_mpu_rbar_val & 0xff00) >> 8;
    mpu_rbar_val[2] = (_mpu_rbar_val & 0xff0000) >> 16;
    mpu_rbar_val[3] = (_mpu_rbar_val & 0xff000000) >> 24;
    gdb_write_memory(stat, MPU_RBAR_ADDRESS, 4, mpu_rbar_val);
    
    // disallow writing to this region
    // SIZE=10 (2^10 = 1024; the size of the entire GPIOE block)
    // AP=101 (privileged read only; i.e. should fault when we try to write LED status)
    // XN=1 (no reason this should ever be executed)
    // SRD = 0 (all subregions have the access policy enforced)
    // TEXCB = 00001 (shared device)
    uint8_t mpu_rasr_val[4];
    mpu_rasr_val[0]  =  (5 << 1) | 1; // size and enable the region
    mpu_rasr_val[1]  =  0;            // SRD
    mpu_rasr_val[2]  =  1;            // TEXCB
    mpu_rasr_val[3]  =  (1 << 4) | 5; // AP and XN
    gdb_write_memory(stat, MPU_RASR_ADDRESS, 4, mpu_rasr_val);
    
    // enable MPU;
    // ENABLE (0) => set
    // HFNMIENA (1) => clear; the handlers can do what they want,
    //                        partially since _we_ run in the handler
    //
    // PRIVDEFENA (2) => set; _everything_ runs in privileged mode,
    //                   so we should fault if the CPU violates
    //                   our memory access policy
    uint8_t mpu_ctrl_val[4];
    gdb_read_memory(stat, MPU_CTRL_ADDRESS, 4, mpu_ctrl_val);
    mpu_ctrl_val[0] |= 5; // sets bits 0 and 2 in the LSB
    gdb_write_memory(stat, MPU_CTRL_ADDRESS, 4, mpu_ctrl_val);
    
    return true;
}
