#include "../include/system_monitor.h"
#include "../include/shared_memory.h"

static volatile sig_atomic_t running = 1;

void signal_handler(int signum) {
    if (signum == SIGINT) {
        running = 0;
    }
}

int main(int argc, char *argv[]) {
    MonitorConfig config;
    SharedData *shared_data;
    sem_t *sem;
    CPUStats prev_cpu_stats;
    DiskStats prev_disk_stats;
    ProcessInfo *prev_processes = NULL;
    int process_count = 0;

    // Parse command line arguments
    if (parse_arguments(argc, argv, &config) != 0) {
        return 1;
    }

    // Set up signal handler
    signal(SIGINT, signal_handler);

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

    // Allocate memory for previous process states
    if (config.monitor_processes) {
        prev_processes = calloc(MAX_PROCESSES, sizeof(ProcessInfo));
        if (!prev_processes) {
            fprintf(stderr, "Failed to allocate memory for process monitoring\n");
            destroy_shared_memory(shared_data);
            close_semaphore(sem);
            destroy_semaphore();
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
        get_process_list(prev_processes, &process_count, MAX_PROCESSES);
    }

    printf("Data collector started (Press Ctrl+C to exit)\n");

    // Main collection loop
    while (running) {
        printf("Collector process: Waiting for semaphore...\n");
        // Wait for semaphore
        sem_wait(sem);
        printf("Collector process: Got semaphore\n");

        // Collect CPU stats
        if (config.monitor_cpu) {
            read_cpu_stats(&shared_data->cpu_stats);
        }

        // Collect memory stats
        if (config.monitor_memory) {
            read_memory_stats(&shared_data->memory_stats);
        }

        // Collect disk stats
        if (config.monitor_disk) {
            read_disk_stats(config.disk_device, &shared_data->disk_stats);
        }

        // Collect process stats
        if (config.monitor_processes) {
            get_process_list(shared_data->processes, &shared_data->process_count, MAX_PROCESSES);
            
            // Calculate CPU usage for processes
            for (int i = 0; i < shared_data->process_count; i++) {
                for (int j = 0; j < process_count; j++) {
                    if (shared_data->processes[i].pid == prev_processes[j].pid) {
                        calculate_proc_cpu_usage(&prev_processes[j], 
                                              &shared_data->processes[i],
                                              shared_data->cpu_stats.user + 
                                              shared_data->cpu_stats.system - 
                                              prev_cpu_stats.user - 
                                              prev_cpu_stats.system);
                        break;
                    }
                }
            }

            // Update previous process states
            memcpy(prev_processes, shared_data->processes, 
                   shared_data->process_count * sizeof(ProcessInfo));
            process_count = shared_data->process_count;
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
        sleep(config.update_interval);
    }

    // Cleanup
    if (prev_processes) {
        free(prev_processes);
    }
    destroy_shared_memory(shared_data);
    close_semaphore(sem);
    destroy_semaphore();

    printf("\nData collector terminated\n");
    return 0;
} 