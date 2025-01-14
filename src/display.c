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

    // Parse command line arguments
    if (parse_arguments(argc, argv, &config) != 0) {
        return 1;
    }

    // Set up signal handler
    signal(SIGINT, signal_handler);

    // Attach to shared memory
    shared_data = attach_shared_memory();
    if (!shared_data) {
        fprintf(stderr, "Failed to attach to shared memory\n");
        return 1;
    }

    // Open semaphore
    sem = open_semaphore();
    if (!sem) {
        fprintf(stderr, "Failed to open semaphore\n");
        munmap(shared_data, sizeof(SharedData));
        return 1;
    }

    printf("Display process started (Press Ctrl+C to exit)\n");

    // Main display loop
    while (running) {
        // Wait for semaphore
        sem_wait(sem);

        if (shared_data->data_ready) {
            // Clear screen
            printf("\033[2J\033[H");
            printf("System Monitor (Press Ctrl+C to exit)\n");
            printf("----------------------------------------\n");

            // Display CPU information
            if (config.monitor_cpu) {
                float cpu_usage = calculate_cpu_usage(NULL, &shared_data->cpu_stats);
                print_cpu_info(cpu_usage);
            }

            // Display memory information
            if (config.monitor_memory) {
                print_memory_info(&shared_data->memory_stats);
            }

            // Display disk information
            if (config.monitor_disk) {
                float read_speed = 0.0, write_speed = 0.0;
                calculate_disk_usage(NULL, &shared_data->disk_stats, &read_speed, &write_speed);
                print_disk_info(read_speed, write_speed);
            }

            // Display process information
            if (config.monitor_processes) {
                // Sort processes by CPU usage
                qsort(shared_data->processes, shared_data->process_count, 
                      sizeof(ProcessInfo), compare_processes);

                // Print process information
                print_process_header();
                int display_count = (shared_data->process_count < config.num_processes) ? 
                                   shared_data->process_count : config.num_processes;
                
                for (int i = 0; i < display_count; i++) {
                    print_process_info(&shared_data->processes[i]);
                }
            }
        }

        // Release semaphore
        sem_post(sem);

        // Small delay to prevent too frequent updates
        usleep(100000); // 100ms
    }

    // Cleanup
    munmap(shared_data, sizeof(SharedData));
    close_semaphore(sem);

    printf("\nDisplay process terminated\n");
    return 0;
} 