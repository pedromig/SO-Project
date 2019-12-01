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

    while (1) {

    }
}
