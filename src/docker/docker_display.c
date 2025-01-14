#include "monitor.h"

void print_docker_header(void) {
    printf("\n=== Docker Containers ===\n");
    printf("%-12s %-20s %-10s %-12s %-12s %-12s\n",
           "CONTAINER ID", "NAME", "CPU", "MEMORY", "MEM %", "BLOCK I/O");
    printf("--------------------------------------------------------------------------------\n");
}

void print_docker_container_info(docker_stats_t *stats) {
    if (!stats) return;

    // Format memory values
    char memory_str[32];
    snprintf(memory_str, sizeof(memory_str), "%.1fMB", stats->memory_usage / (1024.0 * 1024.0));

    // Format block I/O values
    char block_io_str[32];
    snprintf(block_io_str, sizeof(block_io_str), "%.1fMB/%.1fMB",
             stats->block_read / (1024.0 * 1024.0),
             stats->block_write / (1024.0 * 1024.0));

    printf("%-12.12s %-20.20s %-10.2f %-12s %-12.2f %-12s\n",
           stats->id,
           stats->name,
           stats->cpu_usage,
           memory_str,
           stats->memory_percentage,
           block_io_str);
}

void print_docker_stats_list(docker_stats_t *stats, int count) {
    if (!stats || count <= 0) return;

    print_docker_header();
    for (int i = 0; i < count; i++) {
        print_docker_container_info(&stats[i]);
    }
    printf("\n");
} 