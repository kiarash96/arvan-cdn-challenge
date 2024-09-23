/**
 * @file barrier.h
 * @brief Public interface for a barrier synchronization primitive
 */

#ifndef BARRIER_H
#define BARRIER_H

/** Synchronize access of multiple threads or processes at a single point of execution */
typedef struct {
    int total;      ///< Total number of processes
    int ready;      ///< Number of processes ready to proceed
    sem_t lock;     ///< Semaphore used for protecting read/write access to total and ready
    sem_t sync;     ///< Semaphore used for waiting on barrier
} barrier_t;

/**
 * @brief Initialize barrier structure
 *
 * @param[in] barrier   Pointer to barrier structure
 * @param[in] total     Initial total number of processes that are going to use this barrier
 *
 * @return 0 on success, otherwise returns non-zero and sets errno to indicate error
 */
int barrier_init(barrier_t *barrier, int total);

/**
 * @brief Cleanup barrier structure and free its resources
 *
 * @param[in] barrier   Pointer to barrier structure
 *
 * @return 0 on success, otherwise returns non-zero and sets errno to indicate error
 */
int barrier_cleanup(barrier_t *barrier);

/**
 * @brief Signal the calling process has reached the barrier point
 *
 * @param[in] barrier   Pointer to barrier structure
 *
 * @return 0 on success, otherwise returns non-zero and sets errno to indicate error
 */
int barrier_signal_ready(barrier_t *barrier);

/**
 * @brief Signal the calling process is exiting and total number of processes using this barrier has decreased by one
 *
 * @param[in] barrier   Pointer to barrier structure
 *
 * @return 0 on success, otherwise returns non-zero and sets errno to indicate error
 */
int barrier_signal_exit(barrier_t *barrier);

/**
 * @brief Block until all running processes using this barrier signal ready
 *
 * @param[in] barrier   Pointer to barrier structure
 *
 * @return 0 on success, otherwise returns non-zero and sets errno to indicate error
 */
int barrier_wait_for_all(barrier_t *barrier);

#endif // BARRIER_H
