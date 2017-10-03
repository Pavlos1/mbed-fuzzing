#include <pthread.h>

#include "launcher.h"


typedef struct {
    uint32_t thread_id;
    ExecData * elf_data;
} ThreadPayload;

typedef struct {
    pthread_t desc;
    ThreadPayload payload;
} ThreadInfo;


void * simple_client(void * payload);
void start_workers(uint32_t num_threads);
