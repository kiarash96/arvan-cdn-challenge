#include <semaphore.h>

#include <stdbool.h>
#include <stdlib.h>

#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit/munit.h"

#include "barrier.h"
#include "repairmen.h"

static MunitResult test_shared_mem_init(const MunitParameter params[], void *data) {
    shared_mem_t *mem = malloc(sizeof(shared_mem_t));
    if (!mem)
        return MUNIT_ERROR;

    int status = initialize_shared_mem(mem);
    assert_int(status, ==, 0);

    int total_broken = 0;
    for (int i = 0; i < GRID_SIZE; ++i)
        for (int j = 0; j < GRID_SIZE; ++j)
            total_broken += !mem->grid[i][j].fixed;
    assert_int(total_broken, ==, mem->total_broken);

    cleanup_shared_mem(mem);
    free(mem);
    return MUNIT_OK;
}

static MunitResult test_start_pos(const MunitParameter params[], void *data) {
    int pos[AGENT_COUNT][2];
    initialize_starting_pos(pos);

    for (int i = 0; i < AGENT_COUNT; ++i) {
        assert_int(pos[i][0], ==, STARTING_POS[i][0]);
        assert_int(pos[i][1], ==, STARTING_POS[i][1]);
        assert_true(pos[i][0] == 0 || pos[i][0] == GRID_SIZE-1);
        assert_true(pos[i][1] == 0 || pos[i][1] == GRID_SIZE-1);
    }

    return MUNIT_OK;
}

static MunitResult test_apply_move(const MunitParameter params[], void *data) {
    int pos[2] = {
        strtol(munit_parameters_get(params, "x"), NULL, 0),
        strtol(munit_parameters_get(params, "y"), NULL, 0)
    };

    int dir = strtol(munit_parameters_get(params, "dir"), NULL, 0);

    int new_pos[2] = {0};
    apply_move(pos, dir, new_pos);

    int distance = abs(new_pos[0] - pos[0]) + abs(new_pos[1] - pos[1]);
    assert_int(distance, ==, 1);

    return MUNIT_OK;
}

static MunitResult test_no_move(const MunitParameter params[], void *data) {
    int pos[2] = {
        strtol(munit_parameters_get(params, "x"), NULL, 0),
        strtol(munit_parameters_get(params, "y"), NULL, 0)
    };

    int new_pos[2] = {0};
    apply_move(pos, 0, new_pos);

    assert_int(new_pos[0], ==, pos[0]);
    assert_int(new_pos[1], ==, pos[1]);

    return MUNIT_OK;
}

static MunitResult test_update_pos_no_move(const MunitParameter params[], void *data) {
    int pos[AGENT_COUNT][2];
    action_t action[AGENT_COUNT];
    int dest[AGENT_COUNT][2];

    // Generate random unique positions for each agent
    for (int i = 0; i < AGENT_COUNT; ++i) {
        bool duplicate;

        do {
            pos[i][0] = munit_rand_int_range(0, GRID_SIZE-1);
            pos[i][1] = munit_rand_int_range(0, GRID_SIZE-1);

            duplicate = false;
            for (int j = 0; j < i; ++j) {
                if (pos[i][0] == pos[j][0] && pos[i][1] == pos[j][1])
                    duplicate = true;
            }
        } while (duplicate);

        action[i] = ACT_MOVE;

        dest[i][0] = pos[i][0];
        dest[i][1] = pos[i][1];
    }

    update_positions(pos, action, dest);

    for (int i = 0; i < AGENT_COUNT; ++i) {
        assert_int(pos[i][0], ==, dest[i][0]);
        assert_int(pos[i][1], ==, dest[i][1]);
    }

    return MUNIT_OK;
}

static MunitResult test_update_pos_conflict_1(const MunitParameter params[], void *data) {
    action_t action[AGENT_COUNT] = {ACT_MOVE, ACT_MOVE, ACT_MOVE, ACT_MOVE};
    int pos[AGENT_COUNT][2]  = {{0, 0}, {0, 1}, {0, 2}, {0, 3}};
    int dest[AGENT_COUNT][2] = {{0, 1}, {0, 2}, {0, 3}, {0, 3}};
    int res[AGENT_COUNT][2]  = {{0, 0}, {0, 1}, {0, 2}, {0, 3}};

    update_positions(pos, action, dest);

    for (int i = 0; i < AGENT_COUNT; ++i) {
        assert_int(pos[i][0], ==, res[i][0]);
        assert_int(pos[i][1], ==, res[i][1]);
    }

    return MUNIT_OK;
}

static MunitResult test_update_pos_conflict_2(const MunitParameter params[], void *data) {
    action_t action[AGENT_COUNT] = {ACT_MOVE, ACT_MOVE, ACT_MOVE, ACT_MOVE};
    int pos[AGENT_COUNT][2]  = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    int dest[AGENT_COUNT][2] = {{0, 1}, {1, 1}, {0, 0}, {1, 0}};
    int res[AGENT_COUNT][2]  = {{0, 1}, {1, 1}, {0, 0}, {1, 0}};

    update_positions(pos, action, dest);

    for (int i = 0; i < AGENT_COUNT; ++i) {
        assert_int(pos[i][0], ==, res[i][0]);
        assert_int(pos[i][1], ==, res[i][1]);
    }

    return MUNIT_OK;
}

static MunitResult test_update_pos_priority_1(const MunitParameter params[], void *data) {
    action_t action[AGENT_COUNT] = {ACT_MOVE, ACT_MOVE, ACT_MOVE, ACT_MOVE};
    int pos[AGENT_COUNT][2]  = {{0, 0}, {0, 1}, {1, 1}, {0, 2}};
    int dest[AGENT_COUNT][2] = {{0, 1}, {0, 2}, {0, 1}, {0, 3}};
    int res[AGENT_COUNT][2]  = {{0, 1}, {0, 2}, {1, 1}, {0, 3}};

    update_positions(pos, action, dest);

    for (int i = 0; i < AGENT_COUNT; ++i) {
        assert_int(pos[i][0], ==, res[i][0]);
        assert_int(pos[i][1], ==, res[i][1]);
    }

    return MUNIT_OK;
}

static MunitResult test_update_pos_priority_2(const MunitParameter params[], void *data) {
    action_t action[AGENT_COUNT] = {ACT_MOVE, ACT_MOVE, ACT_MOVE, ACT_MOVE};
    int pos[AGENT_COUNT][2]  = {{1, 1}, {0, 1}, {0, 0}, {0, 2}};
    int dest[AGENT_COUNT][2] = {{0, 1}, {0, 2}, {0, 1}, {0, 3}};
    int res[AGENT_COUNT][2]  = {{0, 1}, {0, 2}, {0, 0}, {0, 3}};

    update_positions(pos, action, dest);

    for (int i = 0; i < AGENT_COUNT; ++i) {
        assert_int(pos[i][0], ==, res[i][0]);
        assert_int(pos[i][1], ==, res[i][1]);
    }

    return MUNIT_OK;
}

static MunitResult test_update_pos_no_conflict(const MunitParameter params[], void *data) {
    action_t action[AGENT_COUNT] = {ACT_MOVE, ACT_MOVE, ACT_MOVE, ACT_MOVE};
    int pos[AGENT_COUNT][2] = {{0, 0}, {2, 3}, {5, 1}, {4, 2}};
    int dest[AGENT_COUNT][2] = {{0, 1}, {1, 3}, {4, 1}, {4, 2}};

    update_positions(pos, action, dest);

    for (int i = 0; i < AGENT_COUNT; ++i) {
        assert_int(pos[i][0], ==, dest[i][0]);
        assert_int(pos[i][1], ==, dest[i][1]);
    }

    return MUNIT_OK;
}

static MunitResult test_update_pos_act_die(const MunitParameter params[], void *data) {
    int pos[AGENT_COUNT][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
    action_t action[AGENT_COUNT] = {ACT_MOVE, ACT_DIE, ACT_DIE, ACT_DIE};
    int dest[AGENT_COUNT][2] = {{1, 0}, {1, 0}, {1, 0}, {1, 0}};

    update_positions(pos, action, dest);

    // The first agent can move freely because other agents are dead
    assert_int(pos[0][0], ==, dest[0][0]);
    assert_int(pos[0][1], ==, dest[0][1]);

    return MUNIT_OK;
}

static char* x_params[] = {"0", "6", NULL};
static char* y_params[] = {"0", "6", NULL};
static char* dir_params[] = {"1", "2", "3", "4", NULL};

static MunitParameterEnum apply_move_params[] = {
    {"x", x_params},
    {"y", y_params},
    {"dir", dir_params},
    {NULL, NULL}
};

static MunitParameterEnum no_move_params[] = {
    {"x", x_params},
    {"y", y_params},
    {NULL, NULL}
};

static MunitTest tests[] = {
    {"/test_shared_mem_init", test_shared_mem_init, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_start_pos", test_start_pos, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_apply_move", test_apply_move, NULL, NULL, MUNIT_TEST_OPTION_NONE, apply_move_params},
    {"/test_no_move", test_no_move, NULL, NULL, MUNIT_TEST_OPTION_NONE, no_move_params},
    {"/test_update_pos_no_move", test_update_pos_no_move, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_update_pos_no_conflict", test_update_pos_no_conflict, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_update_pos_conflict_1", test_update_pos_conflict_1, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_update_pos_conflict_2", test_update_pos_conflict_2, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_update_pos_priority_1", test_update_pos_priority_1, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_update_pos_priority_2", test_update_pos_priority_2, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_update_pos_act_die", test_update_pos_act_die, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}
};

static MunitSuite suite = {
    "/repairmen_tests",         // Test suite name
    tests,                      // Tests in this suite
    NULL,                       // No sub-suites
    1,                          // Number of iterations
    MUNIT_SUITE_OPTION_NONE     // Options
};

int main(int argc, char *argv[]) {
    return munit_suite_main(&suite, NULL, argc, argv);
}
