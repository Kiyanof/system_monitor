#include "../../include/disk_monitor.h"

// Read disk I/O statistics from /proc/diskstats
int read_disk_stats(const char *device, DiskStats *stats) {
    FILE *fp;
    char line[512];
    char current_device[MAX_DISK_NAME_LEN];
    
    fp = fopen("/proc/diskstats", "r");
    if (fp == NULL) {
        perror("Error opening /proc/diskstats");
        return -1;
    }

    // Read disk statistics line by line
    while (fgets(line, sizeof(line), fp)) {
        // Parse disk statistics
        // Format: major minor name reads_completed reads_merged sectors_read ms_reading writes_completed writes_merged sectors_written ms_writing
        if (sscanf(line, "%*d %*d %s %lu %lu %lu %lu %lu %lu %lu %lu",
                   current_device,
                   &stats->reads_completed,
                   &stats->reads_merged,
                   &stats->sectors_read,
                   &stats->time_reading,
                   &stats->writes_completed,
                   &stats->writes_merged,
                   &stats->sectors_written,
                   &stats->time_writing) == 9) {
            
            // Check if this is the device we're looking for
            if (strcmp(current_device, device) == 0) {
                fclose(fp);
                return 0;
            }
        }
    }

    fclose(fp);
    fprintf(stderr, "Device %s not found\n", device);
    return -1;
}

// Calculate disk I/O speeds
void calculate_disk_usage(DiskStats *prev, DiskStats *current, float *read_speed, float *write_speed) {
    unsigned long sectors_read_diff = current->sectors_read - prev->sectors_read;
    unsigned long sectors_written_diff = current->sectors_written - prev->sectors_written;
    
    // Convert sectors to megabytes (sector = 512 bytes)
    *read_speed = (float)(sectors_read_diff * 512) / (1024 * 1024);
    *write_speed = (float)(sectors_written_diff * 512) / (1024 * 1024);
}

// Print disk I/O information
void print_disk_info(float read_speed, float write_speed) {
    printf("\nDisk I/O Information:\n");
    printf("Read Speed: %.2f MB/s\n", read_speed);
    printf("Write Speed: %.2f MB/s\n", write_speed);
    printf("Total I/O: %.2f MB/s\n", read_speed + write_speed);
} 