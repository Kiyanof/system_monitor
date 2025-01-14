#include "../include/monitor.h"
#include <pwd.h>

// Print program usage information
void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n\n", program_name);
    printf("Options:\n");
    printf("  -c, --cpu              Monitor CPU usage\n");
    printf("  -m, --memory           Monitor memory usage\n");
    printf("  -d, --disk DEVICE      Monitor disk I/O (e.g., -d sda)\n");
    printf("  -p, --processes N      Monitor top N processes (default: 5)\n");
    printf("  -a, --all              Monitor all metrics\n");
    printf("  -i, --interval N       Set update interval to N seconds (default: 1)\n");
    printf("  -h, --help             Display this help message\n\n");
    printf("If no monitoring options are specified, all metrics will be monitored.\n");
    printf("Default disk device is 'sda' when monitoring all metrics.\n");
}

// Parse command line arguments
void parse_arguments(int argc, char *argv[], MonitorConfig *config) {
    static struct option long_options[] = {
        {"cpu",       no_argument,       0, 'c'},
        {"memory",    no_argument,       0, 'm'},
        {"disk",      required_argument, 0, 'd'},
        {"processes", required_argument, 0, 'p'},
        {"all",       no_argument,       0, 'a'},
        {"interval",  required_argument, 0, 'i'},
        {"help",      no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    // Set default values
    config->monitor_cpu = false;
    config->monitor_memory = false;
    config->monitor_disk = false;
    config->monitor_processes = false;
    config->num_processes = 5;  // Default to showing top 5 processes
    config->update_interval = 1;
    strncpy(config->disk_device, "sda", MAX_DISK_NAME_LEN - 1);

    int option;
    int option_index = 0;

    while ((option = getopt_long(argc, argv, "cmd:p:ai:h", long_options, &option_index)) != -1) {
        switch (option) {
            case 'c':
                config->monitor_cpu = true;
                break;
            case 'm':
                config->monitor_memory = true;
                break;
            case 'd':
                config->monitor_disk = true;
                strncpy(config->disk_device, optarg, MAX_DISK_NAME_LEN - 1);
                break;
            case 'p':
                config->monitor_processes = true;
                config->num_processes = atoi(optarg);
                if (config->num_processes < 1) {
                    config->num_processes = 5;
                    fprintf(stderr, "Warning: Invalid process count. Using 5.\n");
                }
                break;
            case 'a':
                config->monitor_cpu = true;
                config->monitor_memory = true;
                config->monitor_disk = true;
                config->monitor_processes = true;
                break;
            case 'i':
                config->update_interval = atoi(optarg);
                if (config->update_interval < 1) {
                    config->update_interval = 1;
                    fprintf(stderr, "Warning: Invalid interval. Using 1 second.\n");
                }
                break;
            case 'h':
                print_usage(argv[0]);
                exit(0);
            case '?':
                print_usage(argv[0]);
                exit(1);
        }
    }

    // If no monitoring options specified, monitor everything
    if (!config->monitor_cpu && !config->monitor_memory && 
        !config->monitor_disk && !config->monitor_processes) {
        config->monitor_cpu = true;
        config->monitor_memory = true;
        config->monitor_disk = true;
        config->monitor_processes = true;
    }
}

// Read CPU statistics from /proc/stat
int read_cpu_stats(CPUStats *stats) {
    FILE *fp;
    char buffer[256];
    
    fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("Error opening /proc/stat");
        return -1;
    }

    // Read the first line which contains CPU statistics
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        perror("Error reading /proc/stat");
        fclose(fp);
        return -1;
    }

    // Parse CPU statistics
    // Format: cpu user nice system idle iowait irq softirq steal guest guest_nice
    sscanf(buffer, "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
           &stats->user, &stats->nice, &stats->system, &stats->idle,
           &stats->iowait, &stats->irq, &stats->softirq, &stats->steal,
           &stats->guest, &stats->guest_nice);

    fclose(fp);
    return 0;
}

// Calculate CPU usage percentage
float calculate_cpu_usage(CPUStats *prev, CPUStats *current) {
    unsigned long prev_idle, curr_idle;
    unsigned long prev_total, curr_total;
    unsigned long total_diff, idle_diff;
    float cpu_usage;

    // Calculate idle time
    prev_idle = prev->idle + prev->iowait;
    curr_idle = current->idle + current->iowait;

    // Calculate total CPU time
    prev_total = prev->user + prev->nice + prev->system + prev->idle +
                 prev->iowait + prev->irq + prev->softirq + prev->steal;
    curr_total = current->user + current->nice + current->system + current->idle +
                 current->iowait + current->irq + current->softirq + current->steal;

    // Calculate the differences
    total_diff = curr_total - prev_total;
    idle_diff = curr_idle - prev_idle;

    // Calculate CPU usage percentage
    if (total_diff == 0) {
        return 0.0;
    }
    
    cpu_usage = 100.0 * (1.0 - ((float)idle_diff / total_diff));
    return cpu_usage;
}

// Print CPU usage information
void print_cpu_info(float usage) {
    printf("CPU Usage: %.2f%%\n", usage);
}

// Read memory statistics from /proc/meminfo
int read_memory_stats(MemoryStats *stats) {
    FILE *fp;
    char line[256];
    unsigned long value;
    
    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("Error opening /proc/meminfo");
        return -1;
    }

    // Initialize all values to 0
    memset(stats, 0, sizeof(MemoryStats));

    // Read memory information line by line
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %lu kB", &value) == 1) {
            stats->total = value;
        } else if (sscanf(line, "MemFree: %lu kB", &value) == 1) {
            stats->free = value;
        } else if (sscanf(line, "MemAvailable: %lu kB", &value) == 1) {
            stats->available = value;
        } else if (sscanf(line, "Buffers: %lu kB", &value) == 1) {
            stats->buffers = value;
        } else if (sscanf(line, "Cached: %lu kB", &value) == 1) {
            stats->cached = value;
        } else if (sscanf(line, "SwapTotal: %lu kB", &value) == 1) {
            stats->swap_total = value;
        } else if (sscanf(line, "SwapFree: %lu kB", &value) == 1) {
            stats->swap_free = value;
        }
    }

    fclose(fp);
    return 0;
}

// Calculate memory usage percentage and other metrics
void calculate_memory_usage(MemoryStats *stats, float *usage_percent) {
    unsigned long used_memory;
    
    // Calculate used memory (excluding buffers/cache)
    used_memory = stats->total - stats->free - stats->buffers - stats->cached;
    
    // Calculate usage percentage
    *usage_percent = 100.0 * ((float)used_memory / stats->total);
}

// Print memory usage information
void print_memory_info(MemoryStats *stats) {
    float usage_percent;
    calculate_memory_usage(stats, &usage_percent);
    
    printf("\nMemory Information:\n");
    printf("Total Memory: %lu MB\n", stats->total / 1024);
    printf("Used Memory: %lu MB\n", (stats->total - stats->free - stats->buffers - stats->cached) / 1024);
    printf("Free Memory: %lu MB\n", stats->free / 1024);
    printf("Buffers: %lu MB\n", stats->buffers / 1024);
    printf("Cached: %lu MB\n", stats->cached / 1024);
    printf("Memory Usage: %.2f%%\n", usage_percent);
    
    if (stats->swap_total > 0) {
        float swap_usage = 100.0 * (float)(stats->swap_total - stats->swap_free) / stats->swap_total;
        printf("Swap Usage: %.2f%% (%lu MB / %lu MB)\n",
               swap_usage,
               (stats->swap_total - stats->swap_free) / 1024,
               stats->swap_total / 1024);
    }
}

// Read disk I/O statistics from /proc/diskstats
int read_disk_stats(const char *device, DiskStats *stats) {
    FILE *fp;
    char line[512];
    char current_device[MAX_DISK_NAME_LEN];
    
    fp = fopen("/proc/diskstats", "r");
    if (fp == NULL) {
        perror("Error opening /proc/diskstats");
        return -1;
    }

    // Read disk statistics line by line
    while (fgets(line, sizeof(line), fp)) {
        // Parse disk statistics
        // Format: major minor name reads_completed reads_merged sectors_read ms_reading writes_completed writes_merged sectors_written ms_writing
        if (sscanf(line, "%*d %*d %s %lu %lu %lu %lu %lu %lu %lu %lu",
                   current_device,
                   &stats->reads_completed,
                   &stats->reads_merged,
                   &stats->sectors_read,
                   &stats->time_reading,
                   &stats->writes_completed,
                   &stats->writes_merged,
                   &stats->sectors_written,
                   &stats->time_writing) == 9) {
            
            // Check if this is the device we're looking for
            if (strcmp(current_device, device) == 0) {
                fclose(fp);
                return 0;
            }
        }
    }

    fclose(fp);
    fprintf(stderr, "Device %s not found\n", device);
    return -1;
}

// Calculate disk I/O speeds
void calculate_disk_usage(DiskStats *prev, DiskStats *current, float *read_speed, float *write_speed) {
    unsigned long sectors_read_diff = current->sectors_read - prev->sectors_read;
    unsigned long sectors_written_diff = current->sectors_written - prev->sectors_written;
    
    // Convert sectors to megabytes (sector = 512 bytes)
    *read_speed = (float)(sectors_read_diff * 512) / (1024 * 1024);
    *write_speed = (float)(sectors_written_diff * 512) / (1024 * 1024);
}

// Print disk I/O information
void print_disk_info(float read_speed, float write_speed) {
    printf("\nDisk I/O Information:\n");
    printf("Read Speed: %.2f MB/s\n", read_speed);
    printf("Write Speed: %.2f MB/s\n", write_speed);
    printf("Total I/O: %.2f MB/s\n", read_speed + write_speed);
}

// Read process statistics from /proc/[pid]/stat
int read_proc_stat(pid_t pid, ProcessInfo *info) {
    FILE *fp;
    char path[256];
    char buffer[1024];
    char *comm_start, *comm_end;
    
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }

    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        fclose(fp);
        return -1;
    }

    // Extract process name from between parentheses
    comm_start = strchr(buffer, '(');
    comm_end = strrchr(buffer, ')');
    if (comm_start && comm_end && comm_start < comm_end) {
        size_t name_len = comm_end - comm_start - 1;
        if (name_len > MAX_PROC_NAME - 1) {
            name_len = MAX_PROC_NAME - 1;
        }
        strncpy(info->name, comm_start + 1, name_len);
        info->name[name_len] = '\0';
    }

    // Parse the rest of the stat file
    sscanf(comm_end + 2, 
           "%c %*d %*d %*d %*d %*d %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %lu",
           &info->state,
           &info->utime,
           &info->stime,
           &info->starttime);

    fclose(fp);
    return 0;
}

// Read process memory information from /proc/[pid]/status
int read_proc_status(pid_t pid, ProcessInfo *info) {
    FILE *fp;
    char path[256];
    char line[256];
    
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        unsigned long value;
        if (strncmp(line, "Uid:", 4) == 0) {
            sscanf(line, "Uid: %u", &info->uid);
        } else if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize: %lu", &info->vm_size);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %lu", &info->memory);
        }
    }

    fclose(fp);
    return 0;
}

// Calculate CPU usage percentage for a process
void calculate_proc_cpu_usage(ProcessInfo *prev, ProcessInfo *curr, unsigned long total_time) {
    unsigned long proc_time = (curr->utime + curr->stime) - (prev->utime + prev->stime);
    if (total_time > 0) {
        curr->cpu_usage = 100.0 * ((float)proc_time / total_time);
    } else {
        curr->cpu_usage = 0.0;
    }
}

// Compare processes by CPU usage for sorting
int compare_processes(const void *a, const void *b) {
    const ProcessInfo *p1 = (const ProcessInfo *)a;
    const ProcessInfo *p2 = (const ProcessInfo *)b;
    if (p1->cpu_usage < p2->cpu_usage) return 1;
    if (p1->cpu_usage > p2->cpu_usage) return -1;
    return 0;
}

// Get list of all processes
int get_process_list(ProcessInfo **list, int *count, int max_processes) {
    DIR *proc_dir;
    struct dirent *entry;
    ProcessInfo *processes;
    int num_processes = 0;
    
    proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("Failed to open /proc");
        return -1;
    }

    // Allocate memory for process list
    processes = malloc(max_processes * sizeof(ProcessInfo));
    if (processes == NULL) {
        closedir(proc_dir);
        return -1;
    }

    // Scan /proc directory for processes
    while ((entry = readdir(proc_dir)) != NULL && num_processes < max_processes) {
        // Check if the entry is a process (directory with numeric name)
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            pid_t pid = atoi(entry->d_name);
            ProcessInfo *curr_proc = &processes[num_processes];
            
            curr_proc->pid = pid;
            
            // Read process information
            if (read_proc_stat(pid, curr_proc) == 0 &&
                read_proc_status(pid, curr_proc) == 0) {
                num_processes++;
            }
        }
    }

    closedir(proc_dir);
    *list = processes;
    *count = num_processes;
    return 0;
}

// Print process list header
void print_process_header(void) {
    printf("\nProcess List (Top CPU Usage):\n");
    printf("%-8s %-20s %-8s %-8s %-8s %-8s %s\n",
           "PID", "NAME", "STATE", "CPU%", "MEM(MB)", "VIRT(MB)", "USER");
    printf("%-8s %-20s %-8s %-8s %-8s %-8s %s\n",
           "--------", "--------------------", "--------", "--------", "--------", "--------", "--------");
}

// Print information for a single process
void print_process_info(ProcessInfo *proc) {
    struct passwd *pw = getpwuid(proc->uid);
    const char *username = pw ? pw->pw_name : "unknown";
    
    printf("%-8d %-20s %-8c %6.1f%% %7lu %8lu %s\n",
           proc->pid,
           proc->name,
           proc->state,
           proc->cpu_usage,
           proc->memory / 1024,  // Convert KB to MB
           proc->vm_size / 1024, // Convert KB to MB
           username);
}

// Print the process list
void print_process_list(ProcessInfo *processes, int count) {
    print_process_header();
    
    // Sort processes by CPU usage
    qsort(processes, count, sizeof(ProcessInfo), compare_processes);
    
    // Print top processes
    for (int i = 0; i < count; i++) {
        print_process_info(&processes[i]);
    }
}
