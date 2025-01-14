#include "../../include/shared_memory.h"

// Create and initialize shared memory segment
SharedData* create_shared_memory(void) {
    int fd;
    SharedData *data;

    // Remove any existing shared memory object first
    shm_unlink(SHM_NAME);

    // Create shared memory object
    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return NULL;
    }

    // Set the size of shared memory object
    if (ftruncate(fd, sizeof(SharedData)) == -1) {
        perror("ftruncate");
        close(fd);
        shm_unlink(SHM_NAME);
        return NULL;
    }

    // Map shared memory object into process address space
    data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        shm_unlink(SHM_NAME);
        return NULL;
    }

    // Initialize shared memory
    memset(data, 0, sizeof(SharedData));
    data->data_ready = false;

    close(fd);
    return data;
}

// Attach to existing shared memory segment
SharedData* attach_shared_memory(void) {
    int fd;
    SharedData *data;
    int retries = 0;
    const int max_retries = 5;

    // Try to open shared memory with retries
    while (retries < max_retries) {
        fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (fd != -1) break;
        
        sleep(1);  // Wait a bit before retrying
        retries++;
    }

    if (fd == -1) {
        perror("shm_open");
        return NULL;
    }

    // Map shared memory object into process address space
    data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return NULL;
    }

    close(fd);
    return data;
}

// Destroy shared memory segment
void destroy_shared_memory(SharedData *data) {
    if (data) {
        munmap(data, sizeof(SharedData));
        shm_unlink(SHM_NAME);
    }
}

// Create and initialize semaphore
sem_t* create_semaphore(void) {
    sem_t *sem;

    // Remove any existing semaphore first
    sem_unlink(SEM_NAME);

    // Create semaphore
    sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return NULL;
    }

    return sem;
}

// Open existing semaphore
sem_t* open_semaphore(void) {
    sem_t *sem;
    int retries = 0;
    const int max_retries = 5;

    // Try to open semaphore with retries
    while (retries < max_retries) {
        sem = sem_open(SEM_NAME, 0);
        if (sem != SEM_FAILED) break;
        
        sleep(1);  // Wait a bit before retrying
        retries++;
    }

    if (sem == SEM_FAILED) {
        perror("sem_open");
        return NULL;
    }

    return sem;
}

// Close semaphore
void close_semaphore(sem_t *sem) {
    if (sem) {
        sem_close(sem);
    }
}

// Destroy semaphore
void destroy_semaphore(void) {
    sem_unlink(SEM_NAME);
} 