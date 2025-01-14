#include "docker_monitor.h"

static CURL *curl;
static char error_buffer[CURL_ERROR_SIZE];

// Callback function to handle Docker API response
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    docker_api_response_t *resp = (docker_api_response_t *)userp;

    if (!resp->data) {
        resp->data = malloc(realsize + 1);
        if (!resp->data) {
            fprintf(stderr, "Failed to allocate initial memory\n");
            return 0;
        }
        resp->size = 0;
    } else {
        char *ptr = realloc(resp->data, resp->size + realsize + 1);
        if (!ptr) {
            fprintf(stderr, "Failed to reallocate memory\n");
            return 0;
        }
        resp->data = ptr;
    }

    memcpy(&(resp->data[resp->size]), contents, realsize);
    resp->size += realsize;
    resp->data[resp->size] = 0;

    return realsize;
}

int init_docker_monitor(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize CURL\n");
        return -1;
    }
    
    curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, "/var/run/docker.sock");
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); // 5 seconds timeout
    
    return 0;
}

void cleanup_docker_monitor(void) {
    if (curl) {
        curl_easy_cleanup(curl);
        curl = NULL;
    }
    curl_global_cleanup();
}

static int get_container_list(char ***container_ids, int *count) {
    docker_api_response_t resp = {0};
    
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost/v1.41/containers/json");
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Failed to get container list: %s\n", error_buffer);
        free(resp.data);
        return -1;
    }

    if (!resp.data) {
        fprintf(stderr, "No data received from Docker API\n");
        return -1;
    }

    printf("Docker API Response: %s\n", resp.data); // Debug output

    struct json_object *root = json_tokener_parse(resp.data);
    if (!root) {
        fprintf(stderr, "Failed to parse JSON response\n");
        free(resp.data);
        return -1;
    }

    if (json_object_get_type(root) != json_type_array) {
        fprintf(stderr, "Unexpected JSON response type (expected array)\n");
        json_object_put(root);
        free(resp.data);
        return -1;
    }

    *count = json_object_array_length(root);
    if (*count == 0) {
        fprintf(stderr, "No containers found\n");
        json_object_put(root);
        free(resp.data);
        return 0;
    }

    *container_ids = calloc(*count, sizeof(char *));
    if (!*container_ids) {
        fprintf(stderr, "Failed to allocate memory for container IDs\n");
        json_object_put(root);
        free(resp.data);
        return -1;
    }
    
    for (int i = 0; i < *count; i++) {
        struct json_object *container = json_object_array_get_idx(root, i);
        struct json_object *id;
        if (!json_object_object_get_ex(container, "Id", &id)) {
            fprintf(stderr, "Container %d missing ID\n", i);
            continue;
        }
        
        const char *id_str = json_object_get_string(id);
        if (id_str) {
            (*container_ids)[i] = strdup(id_str);
            if (!(*container_ids)[i]) {
                fprintf(stderr, "Failed to allocate memory for container ID %d\n", i);
            }
        }
    }

    json_object_put(root);
    free(resp.data);
    return 0;
}

int get_docker_stats(docker_stats_t **stats, int *count) {
    char **container_ids = NULL;
    int container_count = 0;
    
    if (get_container_list(&container_ids, &container_count) < 0) {
        return -1;
    }

    if (container_count == 0) {
        *count = 0;
        *stats = NULL;
        return 0;
    }

    *stats = calloc(container_count, sizeof(docker_stats_t));
    if (!*stats) {
        fprintf(stderr, "Failed to allocate memory for stats\n");
        for (int i = 0; i < container_count; i++) {
            free(container_ids[i]);
        }
        free(container_ids);
        return -1;
    }
    *count = container_count;

    for (int i = 0; i < container_count; i++) {
        if (!container_ids[i]) continue;

        docker_api_response_t resp = {0};
        char url[512];
        snprintf(url, sizeof(url), "http://localhost/v1.41/containers/%s/stats?stream=false", container_ids[i]);
        
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
        
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Failed to get stats for container %s: %s\n", container_ids[i], error_buffer);
            continue;
        }

        if (!resp.data) {
            fprintf(stderr, "No stats data received for container %s\n", container_ids[i]);
            continue;
        }

        struct json_object *root = json_tokener_parse(resp.data);
        if (!root) {
            fprintf(stderr, "Failed to parse JSON response for container %s\n", container_ids[i]);
            free(resp.data);
            continue;
        }

        // Fill container stats
        strncpy((*stats)[i].id, container_ids[i], sizeof((*stats)[i].id) - 1);
        
        struct json_object *name, *cpu_stats, *memory_stats, *blkio_stats;
        
        if (json_object_object_get_ex(root, "name", &name)) {
            const char *name_str = json_object_get_string(name);
            if (name_str) {
                strncpy((*stats)[i].name, name_str + 1, sizeof((*stats)[i].name) - 1); // Skip leading '/'
            }
        }

        // CPU stats
        if (json_object_object_get_ex(root, "cpu_stats", &cpu_stats)) {
            struct json_object *cpu_usage;
            if (json_object_object_get_ex(cpu_stats, "cpu_usage", &cpu_usage)) {
                struct json_object *total_usage;
                if (json_object_object_get_ex(cpu_usage, "total_usage", &total_usage)) {
                    (*stats)[i].cpu_usage = json_object_get_double(total_usage) / 1000000000.0; // Convert to seconds
                }
            }
        }

        // Memory stats
        if (json_object_object_get_ex(root, "memory_stats", &memory_stats)) {
            struct json_object *usage, *limit;
            if (json_object_object_get_ex(memory_stats, "usage", &usage) &&
                json_object_object_get_ex(memory_stats, "limit", &limit)) {
                (*stats)[i].memory_usage = json_object_get_double(usage);
                (*stats)[i].memory_limit = json_object_get_double(limit);
                if ((*stats)[i].memory_limit > 0) {
                    (*stats)[i].memory_percentage = ((*stats)[i].memory_usage / (*stats)[i].memory_limit) * 100.0;
                }
            }
        }

        // Block I/O stats
        if (json_object_object_get_ex(root, "blkio_stats", &blkio_stats)) {
            struct json_object *io_service_bytes_recursive;
            if (json_object_object_get_ex(blkio_stats, "io_service_bytes_recursive", &io_service_bytes_recursive)) {
                if (json_object_get_type(io_service_bytes_recursive) == json_type_array) {
                    int len = json_object_array_length(io_service_bytes_recursive);
                    for (int j = 0; j < len; j++) {
                        struct json_object *io_stat = json_object_array_get_idx(io_service_bytes_recursive, j);
                        struct json_object *op, *value;
                        if (json_object_object_get_ex(io_stat, "op", &op) &&
                            json_object_object_get_ex(io_stat, "value", &value)) {
                            const char *op_str = json_object_get_string(op);
                            if (strcmp(op_str, "Read") == 0) {
                                (*stats)[i].block_read = json_object_get_double(value);
                            } else if (strcmp(op_str, "Write") == 0) {
                                (*stats)[i].block_write = json_object_get_double(value);
                            }
                        }
                    }
                }
            }
        }

        json_object_put(root);
        free(resp.data);
    }

    // Cleanup
    for (int i = 0; i < container_count; i++) {
        free(container_ids[i]);
    }
    free(container_ids);

    return 0;
}

void free_docker_stats(docker_stats_t *stats) {
    free(stats);
} 