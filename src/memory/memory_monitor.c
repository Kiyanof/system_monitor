#include "../../include/memory_monitor.h"

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

// Calculate memory usage percentage
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