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
    printf("incre\n");
    signal(SIGINT,SIG_IGN);
    signal(SIGUSR1,stats_show);
    signal(SIGUSR2,cleanup);
    msg_t message;
    int j = 100;

    while (1) {

        if (msgrcv(msqid, &message, sizeof(message), FLIGHT_THREAD_REQUEST, 0) < 0) {
            log_error(NULL, "Control Tower: Failed to receive message from the flight!", ON);
        }

        message.msg_type = message.answer_msg_type;
        message.slot = j; // DUMMY NUMBER JUST FOR TESTS

        j += 10;

        if (msgsnd(msqid, &message, sizeof(message) - sizeof(long), IPC_NOWAIT)) {
            log_error(NULL, "Failed to send message to the Control Tower", ON);
        }
    }
}

void stats_show(int signo) {
    signal(SIGUSR1,SIG_IGN);
    printf("Show Stats\n");
}

void cleanup(int signo) {
    printf("cleanup\n");
    exit(0);
}
