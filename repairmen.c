#include <unistd.h>
#include <sys/wait.h>

#include <stdio.h>

#include "repairmen.h"

int agent(int id) {
    printf("Agent %d running!\n", id);
    return 0;
}

int main(int argc, char **argv) {

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

    return 0;
}

