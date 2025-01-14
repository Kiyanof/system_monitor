#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <ctype.h>
#include <stdbool.h>
#include <linux/limits.h>

// ANSI color codes
#define COLOR_RESET   "\x1B[0m"
#define COLOR_RED     "\x1B[31m"
#define COLOR_GREEN   "\x1B[32m"
#define COLOR_YELLOW  "\x1B[33m"
#define COLOR_BLUE    "\x1B[34m"
#define COLOR_MAGENTA "\x1B[35m"
#define COLOR_CYAN    "\x1B[36m"
#define COLOR_WHITE   "\x1B[37m"
#define COLOR_BOLD    "\x1B[1m"

#define MAX_PROC_NAME 256
#define MAX_PROCESSES 1024

// Process state descriptions
#define PROC_RUNNING     'R'  // Running
#define PROC_SLEEPING    'S'  // Sleeping (interruptible)
#define PROC_DISK_SLEEP  'D'  // Disk sleep (uninterruptible)
#define PROC_STOPPED     'T'  // Stopped
#define PROC_ZOMBIE      'Z'  // Zombie
#define PROC_IDLE        'I'  // Idle kernel thread

// Thresholds for color coding
#define CPU_HIGH_THRESHOLD  80.0
#define CPU_MED_THRESHOLD  50.0
#define MEM_HIGH_THRESHOLD  80.0
#define MEM_MED_THRESHOLD  50.0

// Enhanced process information structure
typedef struct {
    pid_t pid;
    char name[MAX_PROC_NAME];
    char state;
    unsigned long memory_usage;    // RSS in KB
    float cpu_usage;              // CPU usage percentage
    unsigned long user_time;      // User time
    unsigned long system_time;    // System time
    unsigned long start_time;     // Process start time
    uid_t uid;                   // User ID
    unsigned long virtual_memory; // Virtual memory size
} ProcessInfo;

// Function declarations
int read_proc_stat(pid_t pid, ProcessInfo *info);
int read_proc_status(pid_t pid, ProcessInfo *info);
int read_proc_cmdline(pid_t pid, ProcessInfo *proc);
void calculate_proc_cpu_usage(ProcessInfo *prev, ProcessInfo *curr, unsigned long total_time);
int compare_processes(const void *a, const void *b);
int get_process_list(ProcessInfo *processes, int *count, int max_processes);
void print_process_header(void);
void print_process_info(ProcessInfo *proc);
void print_process_list(ProcessInfo *processes, int count);
const char* get_state_color(char state);
const char* get_state_description(char state);
const char* get_cpu_color(float cpu_usage);
const char* get_memory_color(float memory_percent);
void format_size(unsigned long size_kb, char *buffer, size_t buflen);

#endif // PROCESS_MONITOR_H 