#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>

#include "monitor.h"
#include "docker_monitor.h"

// Shared memory segment name
#define SHM_NAME "/system_monitor_shm"
#define SEM_NAME "/system_monitor_sem"
#define MAX_DOCKER_CONTAINERS 100

// Structure to hold all monitoring data in shared memory
typedef struct {
    CPUStats cpu_stats;
    MemoryStats memory_stats;
    DiskStats disk_stats;
    ProcessInfo processes[MAX_PROCESSES];
    int process_count;
    docker_stats_t docker_stats[MAX_DOCKER_CONTAINERS];
    int docker_count;
    bool data_ready;
} SharedData;

// Function declarations
SharedData* create_shared_memory(void);
SharedData* attach_shared_memory(void);
void destroy_shared_memory(SharedData *data);
sem_t* create_semaphore(void);
sem_t* open_semaphore(void);
void close_semaphore(sem_t *sem);
void destroy_semaphore(void);

#endif // SHARED_MEMORY_H 