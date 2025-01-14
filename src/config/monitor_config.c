#include "../../include/monitor_config.h"
#include <getopt.h>

// Print usage information
void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -h, --help              Display this help message\n");
    printf("  -c, --cpu               Monitor CPU usage\n");
    printf("  -m, --memory            Monitor memory usage\n");
    printf("  -d, --disk DEVICE       Monitor disk I/O for specified device (e.g., sda, nvme0n1)\n");
    printf("  -p, --processes N       Show top N processes (default: 5)\n");
    printf("  -i, --interval N        Update interval in seconds (default: 1)\n");
    printf("  -a, --all              Monitor all metrics (CPU, memory, disk, processes)\n");
    printf("\nExample: %s -a -p 10 -d nvme0n1\n", program_name);
}

// Parse command line arguments
int parse_arguments(int argc, char *argv[], MonitorConfig *config) {
    static struct option long_options[] = {
        {"help",      no_argument,       0, 'h'},
        {"cpu",       no_argument,       0, 'c'},
        {"memory",    no_argument,       0, 'm'},
        {"disk",      required_argument, 0, 'd'},
        {"processes", required_argument, 0, 'p'},
        {"interval",  required_argument, 0, 'i'},
        {"all",       no_argument,       0, 'a'},
        {0, 0, 0, 0}
    };

    // Set default values
    config->monitor_cpu = false;
    config->monitor_memory = false;
    config->monitor_disk = false;
    config->monitor_processes = false;
    config->num_processes = 5;
    config->update_interval = 1;
    config->disk_device[0] = '\0';

    int option;
    while ((option = getopt_long(argc, argv, "hcmd:p:i:a", long_options, NULL)) != -1) {
        switch (option) {
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
                strncpy(config->disk_device, optarg, MAX_DISK_NAME_LEN - 1);
                config->disk_device[MAX_DISK_NAME_LEN - 1] = '\0';
                break;
            case 'p':
                config->monitor_processes = true;
                config->num_processes = atoi(optarg);
                if (config->num_processes <= 0) {
                    fprintf(stderr, "Number of processes must be positive\n");
                    return -1;
                }
                break;
            case 'i':
                config->update_interval = atoi(optarg);
                if (config->update_interval <= 0) {
                    fprintf(stderr, "Update interval must be positive\n");
                    return -1;
                }
                break;
            case 'a':
                config->monitor_cpu = true;
                config->monitor_memory = true;
                config->monitor_disk = true;
                config->monitor_processes = true;
                break;
            default:
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                return -1;
        }
    }

    // If disk monitoring is enabled but no device specified
    if (config->monitor_disk && config->disk_device[0] == '\0') {
        fprintf(stderr, "Disk device must be specified with -d option\n");
        return -1;
    }

    // If no monitoring option is selected
    if (!config->monitor_cpu && !config->monitor_memory && 
        !config->monitor_disk && !config->monitor_processes) {
        fprintf(stderr, "At least one monitoring option must be selected\n");
        return -1;
    }

    return 0;
} 