#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>

#include "controller.h"


/**
 * Computes checksum and sends MI command
 */
char * gdb_transceive_rsp_packet(ExecStatus * stat, char * command) {
    unsigned int checksum = 0;
    for (unsigned int i=0; i<strlen(command); i++) checksum = (checksum + command[i]) & 0xff;
    
    char low_check, high_check;
    
    unsigned int _low_check = checksum & 0xf;
    if (_low_check < 10) low_check = '0' + _low_check;
    else low_check = 'a' + (_low_check - 10);
    
    unsigned int _high_check = (checksum >> 4) & 0xf;
    if (_high_check < 10) high_check = '0' + _high_check;
    else high_check = 'a' + (_high_check - 10);
    
    char * payload = safe_malloc(strlen(command) + 6);
    sprintf(payload, "$%s#%c%c\n", command, high_check, low_check);
    
    DEBUG("Sending RSP packet to GDB: %s", payload);
    bool res = write(stat->fd_stdin, payload, strlen(payload)) >= 0;
    free(payload);
    if (!res) return NULL;
    
    return gdb_read(stat);
}


/**
 * Reads and parses as RSP packet from GDB
 */
char * gdb_read(ExecStatus * stat) {
    // wait for transmission ACK
    char tmp;
    do {
        read(stat->fd_stdout, &tmp, 1);
        
        if (tmp == '-') {
            DEBUG("GDB requested retransmission");
            return NULL;
        }
    } while (tmp != '+');
    DEBUG("Got transmission ACK");

    char * buf = safe_malloc(1024);
    
    // wait for start of message
    do {
        read(stat->fd_stdout, &tmp, 1);
        DEBUG("Dropping character: %c", tmp);
    } while (tmp != '$');
    DEBUG("Starting message read...");
    
    // and read the rest of the packet
    unsigned int size = read(stat->fd_stdout, buf, 1023);
    buf[1023] = '\0';
    DEBUG("Candidate message read: %s", buf);
    
    if (size < 3) {
        DEBUG("RSP packet too short");
        DEBUG("Got response: %s", buf);
        free(buf);
        return NULL;
    }
    
    unsigned int checksum = 0, checksum_assert = 0;
    for (unsigned int i=0; i < size - 2; i++) {
        if (buf[i] == '#') {
            DEBUG("Found termination character");
            
            if (i == 0) {
                DEBUG("Unsupported command");
                free(buf);
                return NULL;
            }
            
            unsigned int msg_size = i;
            
            buf[i++] = '\0';
            checksum &= 0xff;
            
            unsigned int checksum_assert_high = from_hex_digit(buf[i++]);
            unsigned int checksum_assert_low = from_hex_digit(buf[i++]);
            if ((checksum_assert_low < 0) || (checksum_assert_high < 0)) {
                DEBUG("Checksum contains invalid characters");
            	free(buf);
            	return NULL;
            }
            
            checksum_assert += checksum_assert_low;
            checksum_assert += checksum_assert_high << 4;
            
            if (checksum != checksum_assert) {
                DEBUG("Checksum incorrect");
                DEBUG("Expecting: %d", checksum);
                DEBUG("Actual: %d", checksum_assert);
                free(buf);
                return NULL;
            }
            
            if (!write(stat->fd_stdin, "+", 1)) {
                DEBUG("Broken pipe?");
                exit(1);
            }
            
            char * buf_realloc = realloc(buf, msg_size);
            if (buf_realloc != buf) {
                DEBUG("Memory re-allocation failed");
                exit(1);
            }
            return buf_realloc;
        }
        
        checksum += buf[i];
    }
    
    DEBUG("Message never terminated");
    free(buf);
    return NULL;
}


/**
 * For MI commands expecting no result
 */
bool gdb_send_rsp_packet(ExecStatus * stat, char * command) {
    char * res = gdb_transceive_rsp_packet(stat, command);
    bool ret = (res != NULL);
    free(res);
    
    return ret;
}


/**
 * Sets a breakpoint at requested label and resumes execution
 * until breakpoint is hit. Breakpoints created this way
 * do not persist, though they may clobber already existing
 * ones.
 */
bool gdb_ffwd_to_label(ExecStatus * stat, Elf32_Addr label) {
    char command[] = "Z1,00000000,2";
    ppr_address_32(&command[3], label);
    
    if (!gdb_send_rsp_packet(stat, command)) {
        DEBUG("Set breakpoint failed");
        return false;
    }
    
    if (!gdb_send_rsp_packet(stat, "c")) {
        DEBUG("Continue to breakpoint failed");
        return false;
    }
    
    command[0] = 'z';
    if (!gdb_send_rsp_packet(stat, command)) {
        DEBUG("Clear breakpoint failed");
        return false;
    }
    
    return true;
}


/**
 * Obtains real register values from hardware and
 * overwrites the local record.
 *
 * TODO: VFP registers?
 */
void gdb_read_registers(ExecStatus * stat) {
    char * real_regs = gdb_transceive_rsp_packet(stat, "g");
    if (!real_regs || (real_regs[0] == 'E') || (strlen(real_regs) < (N_REGS << 3))) {
        FATAL("Register read failed");
        exit(1);
    }
    
    stat->regs_avail = (1 << N_REGS) - 1;
    for (int reg=0; reg < N_REGS; reg++) {
        uint32_t val=0;
        for (int byte=0; byte<4; byte++) {
            uint32_t high_digit = from_hex_digit(real_regs[(reg << 3) + (byte << 1)]);
            uint32_t low_digit  = from_hex_digit(real_regs[(reg << 3) + (byte << 1) + 1]);
            if ((high_digit < 0) || (low_digit < 0)) {
                WARN("Read on register %d failed", reg);
                val=0;
                stat->regs_avail &= ~(1 << reg);
                break;
            }
            val += high_digit << ((byte << 3) + 4);
            val += low_digit << (byte << 3);
        }
        stat->regs[reg] = val;
    }
    
    free(real_regs);
}


/**
 * Writes out the local register modifications to
 * hardware.
 *
 * TODO: VFP registers?
 */
void gdb_write_registers(ExecStatus * stat) {
    char * out = safe_malloc((N_REGS << 3) + 2);
    int i=0;
    out[i++] = 'G';
    
    for (int reg=0; reg < N_REGS; reg++) {
        for (int byte=0; byte<4; byte++) {
            uint32_t byteVal = (stat->regs[reg] & (0xff << (byte << 3))) >> (byte << 3);
            int high = byteVal >> 4;
            int low  = byteVal & 0xf;
            out[i++] = high < 10 ? high + '0' : (high - 0xa) + 'a';
            out[i++] = low  < 10 ? low  + '0' : (low  - 0xa) + 'a';
        }
    }
    out[i++] = '\0';
    
    char * ret = gdb_transceive_rsp_packet(stat, out);
    if (!ret || (ret[0] == 'E')) {
        FATAL("Register write failed");
        exit(1);
    }
    
    free(out);
}

