#include "../../include/cpu_monitor.h"

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

    // Calculate total CPU time
    stats->total_time = stats->user + stats->nice + stats->system + stats->idle +
                       stats->iowait + stats->irq + stats->softirq + stats->steal;

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