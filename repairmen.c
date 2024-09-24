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

#include "barrier.h"
#include "repairmen.h"

int initialize_shared_mem(shared_mem_t *mem) {
    int status = 0;

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

    status = barrier_init(&mem->ready_barrier, AGENT_COUNT);
    if (status != 0)
        return status;

    status = barrier_init(&mem->done_barrier, AGENT_COUNT);
    if (status != 0)
        return status;

    return 0;
}

void cleanup_shared_mem(shared_mem_t *mem) {
    barrier_cleanup(&mem->ready_barrier);
    barrier_cleanup(&mem->done_barrier);
}

void initialize_starting_pos(int pos[][2]) {
    for (int i = 0; i < AGENT_COUNT; ++i) {
        pos[i][0] = STARTING_POS[i][0];
        pos[i][1] = STARTING_POS[i][1];
    }
}

bool is_pos_equal(int first[2], int second[2]) {
    return first[0] == second[0] && first[1] == second[1];
}

void apply_move(int pos[2], int direction, int new_pos[2]) {
    new_pos[0] = pos[0] + MOVE_DELTA[direction][0],
    new_pos[1] = pos[1] + MOVE_DELTA[direction][1];

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
    /**
     * To break ties and avoiding deadlocks, a priority system is implemented.
     * Agents wanting to stay where they are have the highest priority.
     * After that agents with lower index have higher priority over agents with lower index.
     */

    // Make a mutable copy of dest
    int new_pos[AGENT_COUNT][2];
    for (int i = 0; i < AGENT_COUNT; ++i) {
        new_pos[i][0] = dest[i][0];
        new_pos[i][1] = dest[i][1];
    }

    /**
     * At each iteration at least one agent's new_pos is set to its previous position
     * So after at most AGENT_COUNT iterations are conflicts are resolved.
     */
    for (int k = 0; k < AGENT_COUNT; ++k) {

        // Check each destination pair for conflicts
        for (int i = 0; i < AGENT_COUNT; ++i) {
            if (action[i] == ACT_DIE)
                continue;

            for (int j = i+1; j < AGENT_COUNT; ++j) {
                if (action[j] == ACT_DIE)
                    continue;

                if (!is_pos_equal(new_pos[i], new_pos[j]))
                    continue;

                // If an agent wants to stay in its previous position it has priority
                
                if (is_pos_equal(pos[i], new_pos[i])) {
                    // Agent j cannot move because its dest is occupied
                    new_pos[j][0] = pos[j][0];
                    new_pos[j][1] = pos[j][1];
                }
                else if (is_pos_equal(pos[j], new_pos[j])) {
                    // Agent i cannot move because its dest is occupied
                    new_pos[i][0] = pos[i][0];
                    new_pos[i][1] = pos[i][1];
                }

                else { 
                    // Two agents must go to the same new destination
                    // i is always lower than j so it has higher priority
                    // so agent j cannot move because its dest is occupied
                    new_pos[j][0] = pos[j][0];
                    new_pos[j][1] = pos[j][1];
                }

            }
        }
    }

    // Copy new_pos into pos
    for (int i = 0; i < AGENT_COUNT; ++i) {
        pos[i][0] = new_pos[i][0];
        pos[i][1] = new_pos[i][1];
    }
}

int agent(shared_mem_t *mem, int id, int target) {
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
            // Choose direction at random
            int new_pos[2];
            apply_move(pos[id], rand() % DIRECTION_COUNT, new_pos);

            mem->action[id] = ACT_MOVE;
            mem->dest[id][0] = new_pos[0];
            mem->dest[id][1] = new_pos[1];
        }
        else { // Cell needs to be fixed
            mem->action[id] = ACT_REPAIR;
            mem->dest[id][0] = pos[id][0];
            mem->dest[id][1] = pos[id][1];
        }

        if (mem->action[id] == ACT_DIE) {
            printf("Agent %d exited with %d moves and %d fixes\n", id+1, n_moves, fixed[id]);
            barrier_signal_exit(&mem->ready_barrier);
            barrier_signal_exit(&mem->done_barrier);
            break;
        }
        else {
            barrier_signal_ready(&mem->ready_barrier);
        }

        // Signal proposed move and wait for all agents to decide on their next action
        barrier_wait_for_all(&mem->ready_barrier);

        if (mem->action[id] == ACT_REPAIR) {
            cell->fixed = true;
            fixed[id] ++;
        }
        else if (!is_pos_equal(pos[id], mem->dest[id])) {
            n_moves ++;
        }
        cell->log[id] = fixed[id];
        update_positions(pos, mem->action, mem->dest);

        //printf("Agent %d moves=%d fixed=%d pos=(%d,%d)\n", id, n_moves, fixed[id], pos[id][0], pos[id][1]);

        // Signal end of move and wait for all agents to do their move
        barrier_signal_ready(&mem->done_barrier);
        barrier_wait_for_all(&mem->done_barrier);

        usleep((id+1) * 10 * 1000);
    }

    return 0;
}

