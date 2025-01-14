#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure to hold CPU statistics
typedef struct {
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    unsigned long iowait;
    unsigned long irq;
    unsigned long softirq;
    unsigned long steal;
    unsigned long guest;
    unsigned long guest_nice;
    unsigned long total_time;  // Total CPU time
} CPUStats;

// Function declarations
int read_cpu_stats(CPUStats *stats);
float calculate_cpu_usage(CPUStats *prev, CPUStats *current);
void print_cpu_info(float usage);

#endif // CPU_MONITOR_H 