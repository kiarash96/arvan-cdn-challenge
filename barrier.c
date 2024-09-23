/**
 * @file barrier.c
 * @brief Implementation for barrier synchronization primitive
 */

#include <semaphore.h>
#include "barrier.h"

int barrier_init(barrier_t *barrier, int total) {
    int status = 0;

    barrier->total = total;
    barrier->ready = 0;

    status = sem_init(&barrier->lock, 1, 1);
    if (status != 0)
        return status;

    status = sem_init(&barrier->sync, 1, 0);
    if (status != 0)
        return status;

    return 0;
}

int barrier_cleanup(barrier_t *barrier) {
    int status = 0;

    status = sem_destroy(&barrier->lock);
    if (status != 0)
        return status;

    status = sem_destroy(&barrier->sync);
    if (status != 0)
        return status;

    return 0;
}

static int post_if_all_ready(barrier_t *barrier) {
    int status = 0;

    if (barrier->ready == barrier->total) {
        barrier->ready = 0;
        for (int i = 0; i < barrier->total; ++i) {
            status = sem_post(&barrier->sync);
            if (status != 0)
                return status;
        }
    }

    return 0;
}

int barrier_signal_ready(barrier_t *barrier) {
    int status = 0;

    status = sem_wait(&barrier->lock);
    if (status != 0)
        return status;

    barrier->ready ++;
    status = post_if_all_ready(barrier);

    sem_post(&barrier->lock);
    return status;
}

int barrier_signal_exit(barrier_t *barrier) {
    int status = 0;

    status = sem_wait(&barrier->lock);
    if (status != 0)
        return status;

    barrier->total --;
    status = post_if_all_ready(barrier);

    sem_post(&barrier->lock);
    return status;
}

int barrier_wait_for_all(barrier_t *barrier) {
    return sem_wait(&barrier->sync);
}

