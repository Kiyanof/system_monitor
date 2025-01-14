#include "../include/monitor.h"
#include "../include/shared_memory.h"

static volatile sig_atomic_t running = 1;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        running = 0;
    }
}

int main(int argc, char *argv[]) {
    MonitorConfig config;
    SharedData *shared_data = NULL;
    sem_t *sem = NULL;
    CPUStats prev_cpu_stats;
    DiskStats prev_disk_stats;
    ProcessInfo *prev_processes = NULL;
    ProcessInfo *current_processes = NULL;
    docker_stats_t *docker_stats = NULL;
    int process_count = 0;

    printf("Debug: Starting collector...\n");

    // Parse command line arguments
    if (parse_arguments(argc, argv, &config) != 0) {
        fprintf(stderr, "Failed to parse arguments\n");
        return 1;
    }

    printf("Debug: Arguments parsed successfully\n");

    // Initialize Docker monitoring if enabled
    if (config.monitor_docker) {
        printf("Debug: Initializing Docker monitoring...\n");
        if (init_docker_monitor() != 0) {
            fprintf(stderr, "Warning: Failed to initialize Docker monitoring\n");
            config.monitor_docker = false;  // Disable Docker monitoring on error
        } else {
            printf("Debug: Docker monitoring initialized successfully\n");
        }
    }

    // Set up signal handler
    signal(SIGINT, signal_handler);
    printf("Debug: Signal handler set up\n");

    printf("Collector process: Creating shared memory...\n");
    // Create shared memory
    shared_data = create_shared_memory();
    if (!shared_data) {
        fprintf(stderr, "Failed to create shared memory\n");
        return 1;
    }
    printf("Collector process: Successfully created shared memory\n");

    printf("Collector process: Creating semaphore...\n");
    // Create semaphore
    sem = create_semaphore();
    if (!sem) {
        fprintf(stderr, "Failed to create semaphore\n");
        destroy_shared_memory(shared_data);
        return 1;
    }
    printf("Collector process: Successfully created semaphore\n");

    // Initial readings
    printf("Debug: Taking initial readings...\n");
    if (config.monitor_cpu) {
        printf("Debug: Reading initial CPU stats...\n");
        if (read_cpu_stats(&prev_cpu_stats) != 0) {
            fprintf(stderr, "Failed to read initial CPU stats\n");
        }
    }
    if (config.monitor_disk) {
        printf("Debug: Reading initial disk stats for device %s...\n", config.disk_device);
        if (read_disk_stats(config.disk_device, &prev_disk_stats) != 0) {
            fprintf(stderr, "Failed to read initial disk stats\n");
        }
    }
    if (config.monitor_processes) {
        printf("Debug: Reading initial process list...\n");
        if (get_process_list(&prev_processes, &process_count, MAX_PROCESSES) != 0) {
            fprintf(stderr, "Failed to read initial process list\n");
        }
    }
    printf("Debug: Initial readings complete\n");

    printf("Data collector started (Press Ctrl+C to exit)\n");

    // Main collection loop
    while (running) {
        printf("Collector process: Waiting for semaphore...\n");
        // Wait for semaphore
        sem_wait(sem);
        printf("Collector process: Got semaphore\n");

        // Collect CPU stats
        if (config.monitor_cpu) {
            printf("Debug: Collecting CPU stats...\n");
            if (read_cpu_stats(&shared_data->cpu_stats) != 0) {
                fprintf(stderr, "Failed to read CPU stats\n");
            }
        }

        // Collect memory stats
        if (config.monitor_memory) {
            printf("Debug: Collecting memory stats...\n");
            if (read_memory_stats(&shared_data->memory_stats) != 0) {
                fprintf(stderr, "Failed to read memory stats\n");
            }
        }

        // Collect disk stats
        if (config.monitor_disk) {
            printf("Debug: Collecting disk stats...\n");
            if (read_disk_stats(config.disk_device, &shared_data->disk_stats) != 0) {
                fprintf(stderr, "Failed to read disk stats\n");
            }
        }

        // Collect process stats
        if (config.monitor_processes) {
            printf("Debug: Collecting process stats...\n");
            int new_count = 0;
            ProcessInfo *new_processes = NULL;
            if (get_process_list(&new_processes, &new_count, MAX_PROCESSES) == 0) {
                // Calculate CPU usage for processes
                for (int i = 0; i < new_count; i++) {
                    for (int j = 0; j < process_count; j++) {
                        if (new_processes[i].pid == prev_processes[j].pid) {
                            calculate_proc_cpu_usage(&prev_processes[j], 
                                                  &new_processes[i],
                                                  shared_data->cpu_stats.user + 
                                                  shared_data->cpu_stats.system - 
                                                  prev_cpu_stats.user - 
                                                  prev_cpu_stats.system);
                            break;
                        }
                    }
                }

                // Copy to shared memory
                shared_data->process_count = new_count > MAX_PROCESSES ? MAX_PROCESSES : new_count;
                memcpy(shared_data->processes, new_processes, 
                       shared_data->process_count * sizeof(ProcessInfo));

                // Update previous process states
                if (prev_processes) {
                    free(prev_processes);
                }
                prev_processes = new_processes;
                process_count = new_count;
            } else {
                fprintf(stderr, "Failed to get process list\n");
                if (new_processes) {
                    free(new_processes);
                }
            }
        }

        // Collect Docker stats
        if (config.monitor_docker) {
            printf("Debug: Collecting Docker stats...\n");
            if (get_docker_stats(&docker_stats, &shared_data->docker_count) == 0) {
                printf("Debug: Got Docker stats for %d containers\n", shared_data->docker_count);
                // Copy stats to shared memory
                int count = shared_data->docker_count;
                if (count > MAX_DOCKER_CONTAINERS) {
                    count = MAX_DOCKER_CONTAINERS;
                }
                memcpy(shared_data->docker_stats, docker_stats, 
                       count * sizeof(docker_stats_t));
                
                // Free temporary stats
                free_docker_stats(docker_stats);
                docker_stats = NULL;
            } else {
                fprintf(stderr, "Failed to get Docker stats\n");
                shared_data->docker_count = 0;
            }
        }

        // Update previous stats
        if (config.monitor_cpu) {
            prev_cpu_stats = shared_data->cpu_stats;
        }
        if (config.monitor_disk) {
            prev_disk_stats = shared_data->disk_stats;
        }

        // Mark data as ready
        shared_data->data_ready = true;
        printf("Collector process: Data is ready\n");

        // Release semaphore
        sem_post(sem);
        printf("Collector process: Released semaphore\n");

        // Sleep for update interval
        printf("Debug: Sleeping for %d seconds...\n", config.update_interval);
        sleep(config.update_interval);
    }

    printf("Debug: Cleaning up...\n");
    // Cleanup
    if (config.monitor_docker) {
        cleanup_docker_monitor();
    }

    if (prev_processes) {
        free(prev_processes);
    }
    if (docker_stats) {
        free_docker_stats(docker_stats);
    }
    destroy_shared_memory(shared_data);
    close_semaphore(sem);
    destroy_semaphore();

    printf("\nData collector terminated\n");
    return 0;
} 