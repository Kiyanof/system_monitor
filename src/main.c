#include "../include/system_monitor.h"

// Global flag for handling Ctrl+C
volatile sig_atomic_t running = 1;

// Signal handler for Ctrl+C
void signal_handler(int signum) {
    if (signum == SIGINT) {
        running = 0;
    }
}

int main(int argc, char *argv[]) {
    MonitorConfig config;
    CPUStats prev_cpu_stats, current_cpu_stats;
    MemoryStats memory_stats;
    DiskStats prev_disk_stats, current_disk_stats;
    ProcessInfo *prev_processes = NULL, *current_processes = NULL;
    float cpu_usage = 0.0, read_speed = 0.0, write_speed = 0.0;
    int process_count;

    // Parse command line arguments
    if (parse_arguments(argc, argv, &config) != 0) {
        return 1;
    }

    // Set up signal handler for Ctrl+C
    signal(SIGINT, signal_handler);

    // Allocate memory for process information arrays if needed
    if (config.monitor_processes) {
        prev_processes = calloc(config.num_processes, sizeof(ProcessInfo));
        current_processes = calloc(config.num_processes, sizeof(ProcessInfo));
        if (!prev_processes || !current_processes) {
            fprintf(stderr, "Failed to allocate memory for process monitoring\n");
            return 1;
        }
    }

    // Initial readings
    if (config.monitor_cpu) {
        read_cpu_stats(&prev_cpu_stats);
    }
    if (config.monitor_disk) {
        if (read_disk_stats(config.disk_device, &prev_disk_stats) != 0) {
            fprintf(stderr, "Failed to read initial disk statistics\n");
            return 1;
        }
    }
    if (config.monitor_processes) {
        process_count = get_process_list(prev_processes, config.num_processes);
        if (process_count < 0) {
            fprintf(stderr, "Failed to get initial process list\n");
            return 1;
        }
    }

    printf("System Monitor Started (Press Ctrl+C to exit)\n");
    printf("Update interval: %d second(s)\n\n", config.update_interval);

    // Main monitoring loop
    while (running) {
        // Clear screen
        printf("\033[2J\033[H");
        printf("System Monitor (Press Ctrl+C to exit)\n");
        printf("----------------------------------------\n");

        // Monitor CPU
        if (config.monitor_cpu) {
            read_cpu_stats(&current_cpu_stats);
            cpu_usage = calculate_cpu_usage(&prev_cpu_stats, &current_cpu_stats);
            print_cpu_info(cpu_usage);
            prev_cpu_stats = current_cpu_stats;
        }

        // Monitor Memory
        if (config.monitor_memory) {
            read_memory_stats(&memory_stats);
            print_memory_info(&memory_stats);
        }

        // Monitor Disk I/O
        if (config.monitor_disk) {
            read_disk_stats(config.disk_device, &current_disk_stats);
            calculate_disk_usage(&prev_disk_stats, &current_disk_stats, &read_speed, &write_speed);
            print_disk_info(read_speed, write_speed);
            prev_disk_stats = current_disk_stats;
        }

        // Monitor Processes
        if (config.monitor_processes) {
            process_count = get_process_list(current_processes, config.num_processes);
            if (process_count > 0) {
                // Calculate CPU usage for each process
                for (int i = 0; i < process_count; i++) {
                    for (int j = 0; j < process_count; j++) {
                        if (current_processes[i].pid == prev_processes[j].pid) {
                            calculate_proc_cpu_usage(&prev_processes[j], &current_processes[i],
                                                   current_cpu_stats.total_time - prev_cpu_stats.total_time);
                            break;
                        }
                    }
                }

                // Sort processes by CPU usage
                qsort(current_processes, process_count, sizeof(ProcessInfo), compare_processes);

                // Print process information
                print_process_list(current_processes, 
                                 process_count < config.num_processes ? process_count : config.num_processes);

                // Swap process arrays
                ProcessInfo *temp = prev_processes;
                prev_processes = current_processes;
                current_processes = temp;
            }
        }

        sleep(config.update_interval);
    }

    // Cleanup
    if (prev_processes) free(prev_processes);
    if (current_processes) free(current_processes);

    printf("\nSystem Monitor Terminated\n");
    return 0;
}
