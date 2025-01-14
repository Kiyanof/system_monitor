#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <pwd.h>
#include <time.h>
#include <sys/types.h>

// Process state descriptions
#define PROC_RUNNING     'R'
#define PROC_SLEEPING    'S'
#define PROC_DISK_SLEEP  'D'
#define PROC_STOPPED     'T'
#define PROC_ZOMBIE      'Z'
#define PROC_IDLE        'I'

// Maximum process name length
#define MAX_PROC_NAME 256

// CPU usage thresholds for color coding
#define CPU_HIGH_THRESHOLD 80.0
#define CPU_MED_THRESHOLD  50.0

// Memory usage thresholds for color coding
#define MEM_HIGH_THRESHOLD 80.0
#define MEM_MED_THRESHOLD  50.0

// ANSI color codes
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_RESET   "\033[0m"

// Process information structure
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
int read_proc_stat(pid_t pid, ProcessInfo *proc);
int read_proc_status(pid_t pid, ProcessInfo *proc);
int read_proc_cmdline(pid_t pid, ProcessInfo *proc);
void calculate_proc_cpu_usage(ProcessInfo *prev, ProcessInfo *current, unsigned long total_time);
int compare_processes(const void *a, const void *b);
int get_process_list(ProcessInfo **processes, int *count, int max_processes);
void print_process_header(void);
void print_process_info(ProcessInfo *proc);
void print_process_list(ProcessInfo *processes, int count);

#endif // PROCESS_MONITOR_H 