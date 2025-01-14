#include "../../include/process_monitor.h"

// Get color for process state
const char* get_state_color(char state) {
    switch (state) {
        case PROC_RUNNING:    return COLOR_GREEN;
        case PROC_SLEEPING:   return COLOR_BLUE;
        case PROC_DISK_SLEEP: return COLOR_MAGENTA;
        case PROC_STOPPED:    return COLOR_YELLOW;
        case PROC_ZOMBIE:     return COLOR_RED;
        case PROC_IDLE:       return COLOR_CYAN;
        default:             return COLOR_WHITE;
    }
}

// Get description for process state
const char* get_state_description(char state) {
    switch (state) {
        case PROC_RUNNING:    return "Running";
        case PROC_SLEEPING:   return "Sleeping";
        case PROC_DISK_SLEEP: return "Disk Sleep";
        case PROC_STOPPED:    return "Stopped";
        case PROC_ZOMBIE:     return "Zombie";
        case PROC_IDLE:       return "Idle";
        default:             return "Unknown";
    }
}

// Get color for CPU usage
const char* get_cpu_color(float cpu_usage) {
    if (cpu_usage >= CPU_HIGH_THRESHOLD) return COLOR_RED;
    if (cpu_usage >= CPU_MED_THRESHOLD)  return COLOR_YELLOW;
    return COLOR_GREEN;
}

// Get color for memory usage
const char* get_memory_color(float memory_percent) {
    if (memory_percent >= MEM_HIGH_THRESHOLD) return COLOR_RED;
    if (memory_percent >= MEM_MED_THRESHOLD)  return COLOR_YELLOW;
    return COLOR_GREEN;
}

// Format size with appropriate unit (KB, MB, GB)
void format_size(unsigned long size_kb, char *buffer, size_t buflen) {
    if (size_kb >= 1024 * 1024) {
        snprintf(buffer, buflen, "%.1f GB", size_kb / (1024.0 * 1024.0));
    } else if (size_kb >= 1024) {
        snprintf(buffer, buflen, "%.1f MB", size_kb / 1024.0);
    } else {
        snprintf(buffer, buflen, "%lu KB", size_kb);
    }
}

// Read process statistics from /proc/[pid]/stat
int read_proc_stat(pid_t pid, ProcessInfo *proc) {
    char path[256];
    char buffer[1024];
    char state;
    unsigned long utime, stime;
    long starttime;
    FILE *fp;

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (fp == NULL) return -1;

    if (fgets(buffer, sizeof(buffer), fp)) {
        // Parse the stat file
        // Format: pid (comm) state ppid ... utime stime ...
        char *comm_start = strchr(buffer, '(');
        char *comm_end = strrchr(buffer, ')');
        if (comm_start && comm_end && comm_end > comm_start) {
            // Extract process name
            size_t comm_len = comm_end - comm_start - 1;
            if (comm_len > MAX_PROC_NAME - 1) comm_len = MAX_PROC_NAME - 1;
            strncpy(proc->name, comm_start + 1, comm_len);
            proc->name[comm_len] = '\0';

            // Parse the rest of the values after the command name
            sscanf(comm_end + 2, "%c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %ld",
                   &state, &utime, &stime, &starttime);
            
            proc->pid = pid;  // Set the PID
            proc->state = state;
            proc->user_time = utime;
            proc->system_time = stime;
            proc->start_time = starttime;
            return 0;
        }
    }

    fclose(fp);
    return -1;
}

// Read process status information from /proc/[pid]/status
int read_proc_status(pid_t pid, ProcessInfo *proc) {
    char path[256];
    char line[256];
    FILE *fp;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (fp == NULL) return -1;

    // Initialize values to 0
    proc->memory_usage = 0;
    proc->virtual_memory = 0;
    proc->uid = 0;

    while (fgets(line, sizeof(line), fp)) {
        unsigned long value;
        if (strncmp(line, "VmRSS:", 6) == 0) {
            if (sscanf(line, "VmRSS: %lu", &value) == 1) {
                proc->memory_usage = value;
            }
        } else if (strncmp(line, "VmSize:", 7) == 0) {
            if (sscanf(line, "VmSize: %lu", &value) == 1) {
                proc->virtual_memory = value;
            }
        } else if (strncmp(line, "Uid:", 4) == 0) {
            unsigned int uid;
            if (sscanf(line, "Uid: %u", &uid) == 1) {
                proc->uid = uid;
            }
        }
    }

    fclose(fp);
    return 0;
}

// Read process command line from /proc/[pid]/cmdline
int read_proc_cmdline(pid_t pid, ProcessInfo *proc) {
    char path[256];
    FILE *fp;
    size_t len;

    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    fp = fopen(path, "r");
    if (fp == NULL) return -1;

    len = fread(proc->name, 1, sizeof(proc->name) - 1, fp);
    if (len > 0) {
        proc->name[len] = '\0';
        // Replace null bytes with spaces
        for (size_t i = 0; i < len - 1; i++) {
            if (proc->name[i] == '\0') proc->name[i] = ' ';
        }
    }

    fclose(fp);
    return 0;
}

// Calculate CPU usage percentage for a process
void calculate_proc_cpu_usage(ProcessInfo *prev, ProcessInfo *current, unsigned long total_time) {
    if (total_time == 0) {
        current->cpu_usage = 0.0;
        return;
    }

    // Calculate the total time spent by this process
    unsigned long proc_time = (current->user_time + current->system_time) -
                            (prev->user_time + prev->system_time);

    // Get the number of clock ticks per second
    long ticks_per_sec = sysconf(_SC_CLK_TCK);
    if (ticks_per_sec <= 0) ticks_per_sec = 100; // Default to 100 if sysconf fails

    // Convert total_time from CPU ticks to the same unit as process time
    unsigned long total_time_ticks = total_time * ticks_per_sec / 100;

    // Calculate CPU usage as a percentage
    if (total_time_ticks > 0) {
        current->cpu_usage = (float)proc_time * 100.0 / total_time_ticks;
    } else {
        current->cpu_usage = 0.0;
    }
}

// Compare function for sorting processes by CPU usage
int compare_processes(const void *a, const void *b) {
    const ProcessInfo *p1 = (const ProcessInfo *)a;
    const ProcessInfo *p2 = (const ProcessInfo *)b;
    
    if (p2->cpu_usage > p1->cpu_usage) return 1;
    if (p2->cpu_usage < p1->cpu_usage) return -1;
    return 0;
}

// Get list of running processes
int get_process_list(ProcessInfo *processes, int max_processes) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;
    
    dir = opendir("/proc");
    if (dir == NULL) {
        perror("Error opening /proc");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL && count < max_processes) {
        // Check if the entry is a process directory (numeric name)
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            pid_t pid = atoi(entry->d_name);
            
            // Initialize the process info structure
            memset(&processes[count], 0, sizeof(ProcessInfo));
            processes[count].pid = pid;

            if (read_proc_stat(pid, &processes[count]) == 0 &&
                read_proc_status(pid, &processes[count]) == 0) {
                read_proc_cmdline(pid, &processes[count]); // Optional, don't fail if it fails
                count++;
            }
        }
    }

    closedir(dir);
    return count;
}

// Print process list header
void print_process_header() {
    printf("\n%sTop Processes:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("%s%-8s %-24s %-12s %-8s %-12s %-12s %-12s %s%s\n",
           COLOR_BOLD,
           "PID", "NAME", "STATE", "CPU%", "MEM", "VIRT", "USER", "TIME",
           COLOR_RESET);
    printf("%s%s%s\n",
           COLOR_BOLD,
           "--------------------------------------------------------------------------------",
           COLOR_RESET);
}

// Print information for a single process
void print_process_info(ProcessInfo *proc) {
    struct passwd *pw = getpwuid(proc->uid);
    char *username = pw ? pw->pw_name : "unknown";
    unsigned long runtime = (proc->user_time + proc->system_time) / sysconf(_SC_CLK_TCK);
    char mem_str[32], virt_str[32];
    float mem_percent = proc->memory_usage * 100.0 / (1024 * 1024); // Convert to percent of total memory

    // Format memory sizes
    format_size(proc->memory_usage, mem_str, sizeof(mem_str));
    format_size(proc->virtual_memory, virt_str, sizeof(virt_str));

    // Print process information with colors
    printf("%-8d ", proc->pid);
    printf("%-24.24s ", proc->name);
    printf("%s%-2c%-9s%s ", 
           get_state_color(proc->state),
           proc->state,
           get_state_description(proc->state),
           COLOR_RESET);
    printf("%s%6.1f%%%s ", 
           get_cpu_color(proc->cpu_usage),
           proc->cpu_usage,
           COLOR_RESET);
    printf("%s%-12s%s ", 
           get_memory_color(mem_percent),
           mem_str,
           COLOR_RESET);
    printf("%-12s ", virt_str);
    printf("%-12.12s ", username);
    printf("%02lu:%02lu:%02lu\n",
           runtime / 3600,
           (runtime % 3600) / 60,
           runtime % 60);
}

// Print the list of processes
void print_process_list(ProcessInfo *processes, int count) {
    print_process_header();
    for (int i = 0; i < count; i++) {
        print_process_info(&processes[i]);
    }
    printf("\n%sProcess States: %s%s(R)unning %s%s(S)leeping %s%s(D)isk Sleep %s%s(T)stopped %s%s(Z)ombie%s\n",
           COLOR_BOLD,
           COLOR_GREEN, "●", COLOR_BLUE, "●", COLOR_MAGENTA, "●",
           COLOR_YELLOW, "●", COLOR_RED, "●", COLOR_RESET);
} 