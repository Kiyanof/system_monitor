#include "../../include/monitor_config.h"
#include "../../include/monitor.h"
#include <getopt.h>

// Print usage information
void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -h, --help              Display this help message\n");
    printf("  -c, --cpu               Monitor CPU usage\n");
    printf("  -m, --memory            Monitor memory usage\n");
    printf("  -d, --disk DEVICE       Monitor disk I/O for specified device (e.g., sda, nvme0n1)\n");
    printf("  -p, --processes N       Show top N processes (default: 10)\n");
    printf("  -D, --docker            Monitor Docker containers\n");
    printf("  -i, --interval N        Update interval in seconds (default: 2)\n");
    printf("  -a, --all              Monitor all metrics (CPU, memory, disk, processes, docker)\n");
    printf("\nExample: %s -a -p 10 -d nvme0n1\n", program_name);
}

// Parse command line arguments
int parse_arguments(int argc, char *argv[], MonitorConfig *config) {
    static struct option long_options[] = {
        {"help",      no_argument,       0, 'h'},
        {"cpu",       no_argument,       0, 'c'},
        {"memory",    no_argument,       0, 'm'},
        {"disk",      required_argument, 0, 'd'},
        {"processes", optional_argument, 0, 'p'},
        {"docker",    no_argument,       0, 'D'},
        {"interval",  required_argument, 0, 'i'},
        {"all",       no_argument,       0, 'a'},
        {0, 0, 0, 0}
    };

    // Set default values
    config->monitor_cpu = false;
    config->monitor_memory = false;
    config->monitor_disk = false;
    config->monitor_processes = false;
    config->monitor_docker = false;
    config->num_processes = 10;  // Default number of processes to show
    config->update_interval = 2;  // Default update interval in seconds
    strncpy(config->disk_device, "sda", MAX_DISK_NAME_LEN - 1);

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "hcmd:p::Di:a", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                print_usage(argv[0]);
                return 1;
            case 'c':
                config->monitor_cpu = true;
                break;
            case 'm':
                config->monitor_memory = true;
                break;
            case 'd':
                config->monitor_disk = true;
                if (optarg) {
                    strncpy(config->disk_device, optarg, MAX_DISK_NAME_LEN - 1);
                }
                break;
            case 'p':
                config->monitor_processes = true;
                if (optarg) {
                    config->num_processes = atoi(optarg);
                    if (config->num_processes <= 0) {
                        config->num_processes = 10;
                    }
                }
                break;
            case 'D':
                config->monitor_docker = true;
                break;
            case 'i':
                if (optarg) {
                    config->update_interval = atoi(optarg);
                    if (config->update_interval <= 0) {
                        config->update_interval = 2;
                    }
                }
                break;
            case 'a':
                config->monitor_cpu = true;
                config->monitor_memory = true;
                config->monitor_disk = true;
                config->monitor_processes = true;
                config->monitor_docker = true;
                break;
            default:
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                return 1;
        }
    }

    // If no monitoring options specified, monitor everything
    if (!config->monitor_cpu && !config->monitor_memory && 
        !config->monitor_disk && !config->monitor_processes && !config->monitor_docker) {
        config->monitor_cpu = true;
        config->monitor_memory = true;
        config->monitor_disk = true;
        config->monitor_processes = true;
        config->monitor_docker = true;
    }

    return 0;
} 