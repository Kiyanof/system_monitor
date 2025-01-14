#ifndef MONITOR_CONFIG_H
#define MONITOR_CONFIG_H

#include <stdbool.h>
#include "disk_monitor.h"

// Monitoring configuration
typedef struct {
    bool monitor_cpu;
    bool monitor_memory;
    bool monitor_disk;
    bool monitor_processes;
    int num_processes;    // Number of top processes to show
    char disk_device[MAX_DISK_NAME_LEN];
    int update_interval;
} MonitorConfig;

// Function declarations
void print_usage(const char *program_name);
int parse_arguments(int argc, char *argv[], MonitorConfig *config);

#endif // MONITOR_CONFIG_H 