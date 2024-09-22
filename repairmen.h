/**
 * @file librepairmen.h
 * @brief Defines public interface for repairmen grid simulation
 */

#ifndef LIBREPAIRMEN_H_
#define LIBREPAIRMEN_H_

/** Number of repairmen processes */
#define AGENT_COUNT 4

/** Width and height of the network grid */
#define GRID_SIZE 7

/** Number of available directions for a move */
#define DIRECTION_COUNT 5

/** Name of the shared memory file in kernel filesystem */
#define SHM_NAME "/repairmen"

/** (x,y) delta for each move */
static const int MOVE_DELTA[DIRECTION_COUNT][2] = {
    {0,  0},    // Staying stationary
    {0, +1},
    {0, -1},
    {+1, 0},
    {-1, 0}
};

/** Start (x,y) position for each agent */
static const int STARTING_POS[AGENT_COUNT][2] = {
    {0, 0},
    {0, GRID_SIZE-1},
    {GRID_SIZE-1, 0},
    {GRID_SIZE-1, GRID_SIZE-1}
};

/** Actions available to agents at each step */
typedef enum {
    ACT_MOVE,
    ACT_REPAIR,
    ACT_DIE
} action_t;

/** A single cell in the grid */
typedef struct {
    bool fixed;             ///< True if this cell is fixed, false if it needs to be repaired
    int log[AGENT_COUNT];   ///< Last recorded number of cells each agent has fixed when visiting this cell
} cell_t;

/** Data shared between agents */
typedef struct {
    cell_t grid[GRID_SIZE][GRID_SIZE];  ///< The network cells
    int total_broken;                   ///< Total number of cells that need to be fixed in the grid

    action_t action[AGENT_COUNT];   ///< Proposed action for each agent
    int dest[AGENT_COUNT][2];       ///< Proposed destination (x,y) pair for each agent

    int alive_count;    ///< Number of agents still operating on the grid

    int ready_count;    ///< Number of agents that have proposed their next move
    sem_t ready_lock;   ///< Semaphore for protecting ready_count
    sem_t action_sync;  ///< Semaphore for synchronizing when all agents have proposed their next move

    int done_count;     ///< Number of agents that have done their action
    sem_t done_lock;    ///< Semaphore for protecting done_count
    sem_t done_sync;    ///< Semaphore for synchronizing when all agents have done their move
} shared_mem_t;

/**
 * @brief Initialize shared memory for the simulation
 *
 * Sets up the grid with random number of broken and fixed cells, and initializes 
 * synchronization mechanisms for agents.
 *
 * @param[in] mem   Pointer to the shared memory structure
 */
void initialize_shared_mem(shared_mem_t *mem);

/**
 * @brief Cleanup the shared memory
 *
 * @param[in] mem   Pointer to the shared memory structure
 */
void cleanup_shared_mem(shared_mem_t *mem);

/**
 * @brief Simulates the behaviour of a repair agent
 *
 * The agent attempts to repair cells in the grid and moves around based on the simulation rules.
 * When the agent reaches its target repairs or deduces there are no more cells left to repair it returns.
 *
 * @param[in] mem       Pointer to the initialized memory structure shared between agents
 * @param[in] id        Unique identifier for this agent. Must be in range 0 <= id < AGENT_COUNT
 * @param[in] target    Number of cells this agent aims to repair before exiting
 *
 * @return 0 on success
 */
int agent(shared_mem_t *mem, int id, int target);

#endif // LIBREPAIRMEN_H_

