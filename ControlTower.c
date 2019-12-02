#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <signal.h>
#include <unistd.h>

#include "ControlTower.h"
#include "logging.h"


void tower_manager() {
    sem_post(tower_mutex);
    struct sigaction terminate_action, stats_action;
    msg_t message;
    int j = 100;

    terminate_action.sa_handler = cleanup;
    sigemptyset(&terminate_action.sa_mask);
    terminate_action.sa_flags = 0;
    sigaction(SIGINT, &terminate_action, NULL);

    stats_action.sa_handler = stats_show;
    sigemptyset(&stats_action.sa_mask);
    sigdelset(&stats_action.sa_mask, SIGKILL);
    stats_action.sa_flags = 0;
    sigaction(SIGUSR1, &stats_action, NULL);

    while (1) {

        if (msgrcv(msqid, &message, sizeof(message), FLIGHT_THREAD_REQUEST, 0) < 0) {
            log_error(log_file, "Control Tower: Failed to receive message from the flight!", ON);
        }

        message.msg_type = message.answer_msg_type;
        message.slot = j; // DUMMY NUMBER JUST FOR TESTS

        j += 10;

        if (msgsnd(msqid, &message, sizeof(message) - sizeof(long), IPC_NOWAIT)) {
            log_error(log_file, "Failed to send message to the Control Tower", ON);
        }
    }
}

void stats_show(int signo) {
    struct sigaction terminate_action;

    terminate_action.sa_handler = cleanup;
    sigemptyset(&terminate_action.sa_mask);
    terminate_action.sa_flags = 0;
    sigaction(SIGINT, &terminate_action, NULL);

    printf("Show Stats\n");
}

void cleanup(int signo) {
    log_info(log_file, "I was killed by my father", ON);
    exit(0);
}
