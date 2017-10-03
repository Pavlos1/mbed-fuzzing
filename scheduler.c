#include "scheduler.h"


/**
 * Simple demonstration of what a worker thread could be doing
 *
 * TODO: An actual worker thread should be choosing which symbols
 *       to jump to, and setting the correct memory protections
 *       when QEMU returns (e.g. protect the outside of the stack)
 */
void * simple_client(void * _payload) {
    ThreadPayload * payload = (ThreadPayload *) _payload;
    ExecStatus * stat = launch_virtual_stm32_ex(QEMU_SYSTEM_ARM, BIN_FILE, payload->elf_data);
    
    Elf32_Addr main_sym = elf_lookup_symbol(stat->data, "main");
    printf("[Thread %d] Skip to main...\n", payload->thread_id);
    if (!gdb_ffwd_to_label(stat, main_sym)) {
        printf("[Thread %d] Could not skip to main\n", payload->thread_id);
    } else {
        printf("[Thread %d] Got to main\n", payload->thread_id);
        printf("[Thread %d] Continuing...\n", payload->thread_id);
        gdb_send_rsp_packet(stat, "c");
    }
    
    pthread_exit(NULL);
}


/**
 * Spins up worker threads to fuzz the target binary
 *
 * NOTE: Since all threads share memory here, we can cache
 *       symbol data
 *
 * See: https://computing.llnl.gov/tutorials/pthreads/
 *
 * TODO: Once we have a means of giving the QEMU instances network
 *       input, we should figure out what data to give to each one
 *
 *       This will likely involve receiving messages from some scheduler
 *       server, since the entire system should be parallelized across
 *       multiple machines as well
 */
void start_workers(uint32_t num_threads) {
    ExecData * elf_data = safe_malloc(sizeof(ExecData));
    elf_load_symbols(elf_data, ELF_FILE);

    ThreadInfo * workers = safe_malloc(num_threads * sizeof(ThreadInfo));
    for (uint32_t i=0; i<num_threads; i++) {
        workers[i].payload.thread_id = i;
        workers[i].payload.elf_data = elf_data;
        if (pthread_create(&workers[i].desc, NULL, simple_client, &workers[i].payload)) {
            WARN("Creating thread %d failed", i);
        }
    }
}
