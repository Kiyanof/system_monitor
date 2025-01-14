#ifndef DOCKER_MONITOR_H
#define DOCKER_MONITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>

// Structure to hold Docker container stats
typedef struct {
    char id[64];
    char name[256];
    double cpu_usage;
    double memory_usage;
    double memory_limit;
    double memory_percentage;
    double block_read;
    double block_write;
} docker_stats_t;

// Structure to hold response data from Docker API
typedef struct {
    char *data;
    size_t size;
} docker_api_response_t;

// Initialize Docker monitoring
int init_docker_monitor(void);

// Cleanup Docker monitoring resources
void cleanup_docker_monitor(void);

// Get stats for all running containers
int get_docker_stats(docker_stats_t **stats, int *count);

// Free docker stats array
void free_docker_stats(docker_stats_t *stats);

#endif // DOCKER_MONITOR_H 