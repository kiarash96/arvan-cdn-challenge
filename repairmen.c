#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "repairmen.h"

void initialize_shared_mem(shared_mem_t *mem) {
    // Set fixed status for grid cells at random
    for (int i = 0; i < GRID_SIZE; ++i)
        for (int j = 0; j < GRID_SIZE; ++j) {
            mem->grid[i][j].fixed = (bool) (rand() % 2);
        }
}

void cleanup_shared_mem(shared_mem_t *mem) {
    // Nothing needs to be done
}

int agent(shared_mem_t *mem, int id) {
    printf("Agent %d running!\n", id);
    return 0;
}

int main(int argc, char **argv) {
    // Initialize random seed
    srand(time(NULL));

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

    // Spawn child processes
    for (int i = 0; i < AGENT_COUNT; ++i) {
        pid_t pid = fork();
        if (pid == 0)
            return agent(mem, i);
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

