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
    mem->total_broken = 0;
    for (int i = 0; i < GRID_SIZE; ++i)
        for (int j = 0; j < GRID_SIZE; ++j) {
            // Set fixed status for grid cells at random
            bool broken = (bool) (rand() % 2);
            mem->grid[i][j].fixed = !broken;
            mem->total_broken += broken;

            for (int k = 0; k < AGENT_COUNT; ++k)
                mem->grid[i][j].log[k] = 0;
        }

    printf("total_broken=%d\n", mem->total_broken);

    mem->alive_count = AGENT_COUNT;
    mem->ready_count = 0;
    sem_init(&mem->ready_lock, 1, 1);
    sem_init(&mem->action_sync, 1, 0);

    mem->done_count = 0;
    sem_init(&mem->done_lock, 1, 1);
    sem_init(&mem->done_sync, 1, 0);
}

void cleanup_shared_mem(shared_mem_t *mem) {
    sem_destroy(&mem->ready_lock);
    sem_destroy(&mem->action_sync);
    sem_destroy(&mem->done_lock);
    sem_destroy(&mem->done_sync);
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

void update_positions(int pos[][2], action_t action[], int dest[][2]) {
    int new_pos[AGENT_COUNT][2];

    // Unset all positions
    for (int i = 0; i < AGENT_COUNT; ++i)
        new_pos[i][0] = new_pos[i][1] = -1;

    // If any agents wants to stay where it is it is prioritize over moving
    for (int i = 0; i < AGENT_COUNT; ++i)
        if (action[i] == ACT_REPAIR ||
            (action[i] == ACT_MOVE && pos[i][0] == dest[i][0] && pos[i][1] == dest[i][1])) {

            new_pos[i][0] = pos[i][0];
            new_pos[i][1] = pos[i][1];
        }

    // Agents with lower index have higher priority if there's a conflict
    for (int i = 0; i < AGENT_COUNT; ++i) {
        if (action[i] != ACT_MOVE)
            continue;

        bool dest_occupied = false;

        for (int j = 0; j < AGENT_COUNT; ++j)
            if (new_pos[j][0] != -1 && new_pos[j][1] != -1
                && dest[i][0] == new_pos[j][0] && dest[i][1] == new_pos[j][1])
            {
                dest_occupied = true;
                break;
            }

        // The agent stays where it is if its destination is occupied by a higher priority agent
        if (dest_occupied) {
            new_pos[i][0] = pos[i][0];
            new_pos[i][1] = pos[i][1];
        }
        else {
            new_pos[i][0] = dest[i][0];
            new_pos[i][1] = dest[i][1];
        }
    }

    // Copy new_pos into pos
    for (int i = 0; i < AGENT_COUNT; ++i) {
        pos[i][0] = new_pos[i][0];
        pos[i][1] = new_pos[i][1];
    }
}

shared_mem_t *mem = NULL;

void signal_death() {
    sem_wait(&mem->ready_lock);
    mem->alive_count --;
    if (mem->ready_count == mem->alive_count) {
        mem->ready_count = 0;
        for (int i = 0; i < mem->alive_count; ++i)
            sem_post(&mem->action_sync);
    }
    sem_post(&mem->ready_lock);
}

void signal_ready() {
    sem_wait(&mem->ready_lock);
    mem->ready_count ++;
    if (mem->ready_count == mem->alive_count) {
        mem->ready_count = 0;
        for (int i = 0; i < mem->alive_count; ++i)
            sem_post(&mem->action_sync);
    }
    sem_post(&mem->ready_lock);
}

void signal_done() {
    sem_wait(&mem->done_lock);
    mem->done_count ++;
    if (mem->done_count == mem->alive_count) {
        mem->done_count = 0;
        for (int i = 0; i < mem->alive_count; ++i)
            sem_post(&mem->done_sync);
    }
    sem_post(&mem->done_lock);
}

int agent(int id, int target) {
    // Stores number of moves this agent has made
    int n_moves = 0;

    // Stores x,y position for each process
    int pos[AGENT_COUNT][2];
    initialize_starting_pos(pos);

    int fixed[AGENT_COUNT] = {0};

    while (true) {
        // Pointer to the cell we're currently in
        cell_t *cell = &mem->grid[pos[id][0]][pos[id][1]];

        for (int i = 0; i < AGENT_COUNT; ++i)
            if (fixed[i] < cell->log[i])
                fixed[i] = cell->log[i];

        // Check exit condition
        int total_fixed = 0;
        for (int i = 0; i < AGENT_COUNT; ++i)
            total_fixed += fixed[i];
        if (fixed[id] == target || total_fixed == mem->total_broken) {
            mem->action[id] = ACT_DIE;
        }
        else if (cell->fixed) {
            int new_pos[2];
            generate_new_pos(pos[id], new_pos);

            mem->action[id] = ACT_MOVE;
            mem->dest[id][0] = new_pos[0];
            mem->dest[id][1] = new_pos[1];
        }
        else { // Cell needs to be fixed
            mem->action[id] = ACT_REPAIR;
        }

        if (mem->action[id] == ACT_DIE) {
            printf("Agent %d exited with %d moves and %d fixes\n", id+1, n_moves, fixed[id]);
            signal_death();
            break;
        }
        else {
            signal_ready();
        }

        // Signal proposed move and wait for all agents to decide on their next action
        sem_wait(&mem->action_sync);

        if (mem->action[id] == ACT_REPAIR) {
            cell->fixed = true;
            fixed[id] ++;
        }
        else if (pos[id][0] != mem->dest[id][0] || pos[id][1] != mem->dest[id][1]) {
            n_moves ++;
        }
        cell->log[id] = fixed[id];
        update_positions(pos, mem->action, mem->dest);

        //printf("Agent %d moves=%d fixed=%d pos=(%d,%d)\n", id, n_moves, fixed[id], pos[id][0], pos[id][1]);

        // Signal end of move and wait for all agents to do their move
        signal_done();
        sem_wait(&mem->done_sync);

        usleep((id+1) * 10 * 1000);
    }

    return 0;
}

int main(int argc, char **argv) {
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
            return agent(i, targets[i]);
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

