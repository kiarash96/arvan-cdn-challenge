#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "repairmen.h"

int main(int argc, char *argv[]) {
    // Initialize random seed
    srand(time(NULL));

    int targets[AGENT_COUNT];
    if (argc != 1 + AGENT_COUNT) {
        printf("Usage: ./repairmen [target1] [target2] [target3] [target4]\n");
        return -1;
    }

    for (int i = 0; i < AGENT_COUNT; ++i) {
        targets[i] = strtol(argv[i+1], NULL, 0);
        if (targets[i] <= 0) {
            printf("Error: Each target must be a positive integer\n");
            return -1;
        }
    }

    // Create shared memory object
    int fd = shm_open(SHM_NAME,
            O_CREAT | O_RDWR,
            S_IRUSR | S_IWUSR);
    if (fd == -1) {
        printf("shm_open failed: %s\n", strerror(errno));
        return -1;
    }

    // Set shared memory size
    if (ftruncate(fd, sizeof(shared_mem_t)) == -1) {
        printf("ftruncate failed: %s\n", strerror(errno));
        return -1;
    }

    // Map shared memory to address space
    shared_mem_t *mem = mmap(NULL,
            sizeof(shared_mem_t),
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            0);
    if (mem == MAP_FAILED) {
        printf("mmap failed: %s\n", strerror(errno));
        return -1;
    }

    initialize_shared_mem(mem);

    printf("total_broken=%d\n", mem->total_broken);

    // Spawn child processes
    for (int i = 0; i < AGENT_COUNT; ++i) {
        pid_t pid = fork();
        if (pid == 0)
            return agent(mem, i, targets[i]);
    }

    // This only runs in parent

    // Wait for all child processes to exit
    for (int i = 0; i < AGENT_COUNT; ++i)
        wait(NULL);
    printf("All child processes exited.\n");

    // Cleanup and delete shared memory
    cleanup_shared_mem(mem);
    munmap(mem, sizeof(shared_mem_t));
    shm_unlink(SHM_NAME);

    return 0;
}

