#include <semaphore.h>
#include <pthread.h>

#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit/munit.h"

#include "barrier.h"

#define NUM_THREADS 4

static void* setup(const MunitParameter params[], void *data) {
    barrier_t *barrier = malloc(sizeof(barrier_t));
    assert_not_null(barrier);

    int status = barrier_init(barrier, NUM_THREADS);
    assert_int(status, ==, 0);

    return barrier;
}

void teardown(void *data) {
    int status = barrier_cleanup(data);
    assert_int(status, ==, 0);

    free(data);
}

// Thread function to test barrier synchronization
void* thread_func(void* data) {
    barrier_t *barrier = (barrier_t*) data;
    
    barrier_signal_ready(barrier);
    barrier_wait_for_all(barrier);
    
    barrier_signal_exit(barrier);
    
    return NULL;
}

static MunitResult test_barrier_init_cleanup(const MunitParameter params[], void *data) {
    barrier_t *barrier = (barrier_t*) data;
    int status = 0, sem_value = 0;

    assert_int(barrier->total, ==, NUM_THREADS);
    assert_int(barrier->ready, ==, 0);

    status = sem_getvalue(&barrier->lock, &sem_value);
    assert_int(status, ==, 0);
    assert_int(sem_value, ==, 1);

    status = sem_getvalue(&barrier->sync, &sem_value);
    assert_int(status, ==, 0);
    assert_int(sem_value, ==, 0);

    return MUNIT_OK;
}

static MunitResult test_barrier_signal_ready(const MunitParameter params[], void *data) {
    barrier_t *barrier = (barrier_t*) data;
    int status = 0;

    status = barrier_signal_ready(barrier);
    assert_int(status, ==, 0);

    assert_int(barrier->ready, ==, 1);

    return MUNIT_OK;
}

static MunitResult test_barrier_signal_exit(const MunitParameter params[], void *data) {
    barrier_t *barrier = (barrier_t*) data;
    int status = 0;

    status = barrier_signal_exit(barrier);
    assert_int(status, ==, 0);

    assert_int(barrier->total, ==, NUM_THREADS-1);

    return MUNIT_OK;
}

static MunitResult test_barrier_signal(const MunitParameter params[], void *data) {
    barrier_t *barrier = (barrier_t*) data;
    int status = 0;

    for (int i = 0; i < NUM_THREADS; ++i) {
        status = barrier_signal_ready(barrier);
        assert_int(status, ==, 0);
    }

    // Ensure that the barrier reset after all processes signaled ready
    assert_int(barrier->ready, ==, 0);

    for (int i = 0; i < NUM_THREADS; ++i) {
        status = barrier_signal_exit(barrier);
        assert_int(status, ==, 0);
    }

    // Ensure that the total is 0 after all exits
    assert_int(barrier->total, ==, 0);

    return MUNIT_OK;
}

static MunitResult test_barrier_sync(const MunitParameter params[], void* data) {
    barrier_t *barrier = (barrier_t*) data;
    pthread_t threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; ++i)
        pthread_create(&threads[i], NULL, thread_func, barrier);

    for (int i = 0; i < NUM_THREADS; ++i)
        pthread_join(threads[i], NULL);

    munit_assert_int(barrier->total, ==, 0);

    return MUNIT_OK;
}

static MunitTest tests[] = {
    {"/test_barrier_init_cleanup", test_barrier_init_cleanup, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_barrier_signal_ready", test_barrier_signal_ready, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_barrier_signal_exit", test_barrier_signal_exit, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_barrier_signal", test_barrier_signal, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {"/test_barrier_sync", test_barrier_sync, setup, teardown, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}
};

static MunitSuite suite = {
    "/barrier_tests",           // Test suite name
    tests,                      // Tests in this suite
    NULL,                       // No sub-suites
    1,                          // Number of iterations
    MUNIT_SUITE_OPTION_NONE     // Options
};

int main(int argc, char *argv[]) {
    return munit_suite_main(&suite, NULL, argc, argv);
}
