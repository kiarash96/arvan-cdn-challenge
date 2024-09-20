#ifndef REPAIRMEN_H_
#define REPAIRMEN_H_

// Number of repairmen processes
#define AGENT_COUNT 4

// Width and height of the network grid
#define GRID_SIZE 7

// Name of the shared memory file in kernel filesystem
#define SHM_NAME "/repairmen"

// Start position for each repairmen
static const int STARTING_POS[AGENT_COUNT][2] = {
    {0, 0},
    {0, GRID_SIZE-1},
    {GRID_SIZE-1, 0},
    {GRID_SIZE-1, GRID_SIZE-1}
};

// Describes a single cell in the grid
typedef struct {
    // True if this cell is fixed, false if it needs to be repaired
    bool fixed;
} cell_t;

// Describes the data shared between agents
typedef struct {
    // The network cells
    cell_t grid[GRID_SIZE][GRID_SIZE];
} shared_mem_t;

#endif // REPAIRMEN_H_

