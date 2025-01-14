#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure to hold memory information
typedef struct {
    unsigned long total;
    unsigned long free;
    unsigned long available;
    unsigned long buffers;
    unsigned long cached;
    unsigned long swap_total;
    unsigned long swap_free;
} MemoryStats;

// Function declarations
int read_memory_stats(MemoryStats *stats);
void calculate_memory_usage(MemoryStats *stats, float *usage_percent);
void print_memory_info(MemoryStats *stats);

#endif // MEMORY_MONITOR_H 