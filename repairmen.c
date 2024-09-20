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

void initialize_shared_mem(shared_mem_t *mem) {
    // Set fixed status for grid cells at random
    for (int i = 0; i < GRID_SIZE; ++i)
        for (int j = 0; j < GRID_SIZE; ++j) {
            mem->grid[i][j].fixed = (bool) (rand() % 2);
        }

    mem->ready_count = 0;
    sem_init(&mem->ready_lock, 1, 1);
    sem_init(&mem->action_sync, 1, 0);
}

void cleanup_shared_mem(shared_mem_t *mem) {
    sem_destroy(&mem->ready_lock);
    sem_destroy(&mem->action_sync);
}

void initialize_starting_pos(int pos[][2]) {
    for (int i = 0; i < AGENT_COUNT; ++i) {
        pos[i][0] = STARTING_POS[i][0];
        pos[i][1] = STARTING_POS[i][1];
    }
}

void generate_new_pos(int pos[2], int new_pos[2]) {
    // Choose direction at random
    int index = rand() % DIRECTION_COUNT;
    new_pos[0] = pos[0] + MOVE_DELTA[index][0],
    new_pos[1] = pos[1] + MOVE_DELTA[index][1];

    // Reverse the direction if we go out of the bounds
    if (new_pos[0] < 0)
        new_pos[0] += 2;
    if (GRID_SIZE - 1 < new_pos[0])
        new_pos[0] -= 2;
    if (new_pos[1] < 0)
        new_pos[1] += 2;
    if (GRID_SIZE - 1 < new_pos[1])
        new_pos[1] -= 2;
}

shared_mem_t *mem = NULL;

void signal_ready() {
    sem_wait(&mem->ready_lock);
    mem->ready_count ++;
    if (mem->ready_count == AGENT_COUNT) {
        mem->ready_count = 0;
        for (int i = 0; i < AGENT_COUNT; ++i)
            sem_post(&mem->action_sync);
    }
    sem_post(&mem->ready_lock);
}

int agent(int id) {
    // Stores number of moves this agent has made
    int n_moves = 0;

    // Stores x,y position for each process
    int pos[AGENT_COUNT][2];
    initialize_starting_pos(pos);

    while (true) {

        int new_pos[2];
        generate_new_pos(pos[id], new_pos);

        pos[id][0] = new_pos[0];
        pos[id][1] = new_pos[1];

        signal_ready();

        // Wait for all agents to decide on their next action
        sem_wait(&mem->action_sync);

        n_moves ++;
        printf("Agent %d moves=%d pos=(%d,%d)\n", id, n_moves, pos[id][0], pos[id][1]);

        sleep(id+1);
    }

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
    mem = mmap(NULL,
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
            return agent(i);
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

