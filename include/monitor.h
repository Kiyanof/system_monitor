#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <dirent.h>
#include <ctype.h>
#include <pwd.h>
#include <signal.h>
#include <time.h>
#include "docker_monitor.h"
#include "monitor_config.h"
#include "disk_monitor.h"

#define MAX_DISK_NAME_LEN 32
#define MAX_PROC_NAME 256
#define MAX_PROCESSES 1024

// Process state descriptions
#define PROC_RUNNING     'R'
#define PROC_SLEEPING    'S'
#define PROC_DISK_SLEEP  'D'
#define PROC_STOPPED     'T'
#define PROC_ZOMBIE      'Z'

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
} CPUStats;

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

// Enhanced process information structure
typedef struct {
    pid_t pid;
    char name[MAX_PROC_NAME];
    char state;
    unsigned long memory;      // RSS in KB
    float cpu_usage;          // CPU usage percentage
    unsigned long utime;      // User time
    unsigned long stime;      // System time
    unsigned long starttime;  // Process start time
    uid_t uid;               // User ID
    unsigned long vm_size;    // Virtual memory size
} ProcessInfo;

// Function declarations
// CPU monitoring
int read_cpu_stats(CPUStats *stats);
float calculate_cpu_usage(CPUStats *prev, CPUStats *current);

// Memory monitoring
int read_memory_stats(MemoryStats *stats);
void calculate_memory_usage(MemoryStats *stats, float *usage_percent);

// Process monitoring functions
int read_proc_stat(pid_t pid, ProcessInfo *info);
int read_proc_status(pid_t pid, ProcessInfo *info);
int read_proc_cmdline(pid_t pid, char *name, size_t len);
void calculate_proc_cpu_usage(ProcessInfo *prev, ProcessInfo *curr, unsigned long total_time);
int compare_processes(const void *a, const void *b);
int get_process_list(ProcessInfo **list, int *count, int max_processes);
void print_process_header(void);
void print_process_info(ProcessInfo *proc);
void print_process_list(ProcessInfo *processes, int count);

// Utility functions
void print_cpu_info(float usage);
void print_memory_info(MemoryStats *stats);
void print_disk_info(float read_speed, float write_speed);
void print_usage(const char *program_name);

// Docker monitoring functions
void print_docker_header(void);
void print_docker_container_info(docker_stats_t *stats);
void print_docker_stats_list(docker_stats_t *stats, int count);

#endif // SYSTEM_MONITOR_H
