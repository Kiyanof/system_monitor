#include "../include/monitor.h"
#include "../include/monitor_config.h"
#include <signal.h>

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
    docker_stats_t *docker_stats = NULL;
    float cpu_usage = 0.0, read_speed = 0.0, write_speed = 0.0;
    int process_count = 0, docker_count = 0;

    // Parse command line arguments
    if (parse_arguments(argc, argv, &config) != 0) {
        return 1;
    }

    // Set up signal handler for Ctrl+C
    signal(SIGINT, signal_handler);

    // Initialize Docker monitoring if enabled
    if (config.monitor_docker) {
        if (init_docker_monitor() != 0) {
            fprintf(stderr, "Warning: Failed to initialize Docker monitoring\n");
            config.monitor_docker = false;
        }
    }

    // Allocate memory for process information arrays if needed
    if (config.monitor_processes) {
        prev_processes = malloc(config.num_processes * sizeof(ProcessInfo));
        current_processes = malloc(config.num_processes * sizeof(ProcessInfo));
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
        read_disk_stats(config.disk_device, &prev_disk_stats);
    }
    if (config.monitor_processes) {
        get_process_list(&prev_processes, &process_count, config.num_processes);
    }

    printf("System Monitor (Press Ctrl+C to exit)\n");
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
            int new_count = 0;
            get_process_list(&current_processes, &new_count, config.num_processes);

            // Calculate CPU usage for each process
            for (int i = 0; i < new_count; i++) {
                for (int j = 0; j < process_count; j++) {
                    if (current_processes[i].pid == prev_processes[j].pid) {
                        calculate_proc_cpu_usage(&prev_processes[j], 
                                              &current_processes[i],
                                              current_cpu_stats.user + 
                                              current_cpu_stats.system - 
                                              prev_cpu_stats.user - 
                                              prev_cpu_stats.system);
                        break;
                    }
                }
            }

            // Sort processes by CPU usage
            qsort(current_processes, new_count, sizeof(ProcessInfo), compare_processes);

            // Print process information
            print_process_list(current_processes, 
                             new_count < config.num_processes ? new_count : 
                             config.num_processes);

            // Update previous process states
            ProcessInfo *temp = prev_processes;
            prev_processes = current_processes;
            current_processes = temp;
            process_count = new_count;
        }

        // Monitor Docker containers
        if (config.monitor_docker) {
            // Free previous stats if they exist
            if (docker_stats) {
                free_docker_stats(docker_stats);
                docker_stats = NULL;
            }
            
            // Get new stats
            if (get_docker_stats(&docker_stats, &docker_count) == 0) {
                print_docker_stats_list(docker_stats, docker_count);
            } else {
                fprintf(stderr, "Failed to get Docker statistics\n");
            }
        }

        // Sleep for update interval
        sleep(config.update_interval);
    }

    // Cleanup
    if (prev_processes) {
        free(prev_processes);
    }
    if (current_processes) {
        free(current_processes);
    }
    if (docker_stats) {
        free_docker_stats(docker_stats);
    }
    if (config.monitor_docker) {
        cleanup_docker_monitor();
    }

    printf("\nMonitoring terminated\n");
    return 0;
}
