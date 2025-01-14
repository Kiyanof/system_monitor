#ifndef DISK_MONITOR_H
#define DISK_MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DISK_NAME_LEN 32

// Structure to hold disk I/O statistics
typedef struct {
    unsigned long reads_completed;
    unsigned long reads_merged;
    unsigned long sectors_read;
    unsigned long time_reading;
    unsigned long writes_completed;
    unsigned long writes_merged;
    unsigned long sectors_written;
    unsigned long time_writing;
} DiskStats;

// Function declarations
int read_disk_stats(const char *device, DiskStats *stats);
void calculate_disk_usage(DiskStats *prev, DiskStats *current, float *read_speed, float *write_speed);
void print_disk_info(float read_speed, float write_speed);

#endif // DISK_MONITOR_H 