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
    SharedData *shared_data;
    sem_t *sem;
    float cpu_usage = 0.0;
    float read_speed = 0.0, write_speed = 0.0;
    CPUStats prev_cpu_stats = {0};
    DiskStats prev_disk_stats = {0};
    bool first_reading = true;

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
        destroy_shared_memory(shared_data);
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

            // Display CPU stats
            if (config.monitor_cpu) {
                if (!first_reading) {
                    cpu_usage = calculate_cpu_usage(&prev_cpu_stats, &shared_data->cpu_stats);
                }
                print_cpu_info(cpu_usage);
                prev_cpu_stats = shared_data->cpu_stats;
            }

            // Display memory stats
            if (config.monitor_memory) {
                print_memory_info(&shared_data->memory_stats);
            }

            // Display disk stats
            if (config.monitor_disk) {
                if (!first_reading) {
                    calculate_disk_usage(&prev_disk_stats, &shared_data->disk_stats,
                                      &read_speed, &write_speed);
                }
                print_disk_info(read_speed, write_speed);
                prev_disk_stats = shared_data->disk_stats;
            }

            // Display process stats
            if (config.monitor_processes && shared_data->process_count > 0) {
                print_process_list(shared_data->processes, 
                                 shared_data->process_count < config.num_processes ? 
                                 shared_data->process_count : config.num_processes);
            }

            // Display Docker stats
            if (config.monitor_docker && shared_data->docker_count > 0) {
                print_docker_stats_list(shared_data->docker_stats, shared_data->docker_count);
            }

            first_reading = false;
        }

        // Release semaphore
        sem_post(sem);

        // Small delay to prevent too frequent updates
        usleep(100000);  // 100ms
    }

    // Cleanup
    destroy_shared_memory(shared_data);
    close_semaphore(sem);

    printf("\nDisplay process terminated\n");
    return 0;
} 