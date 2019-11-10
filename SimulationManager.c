#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <wait.h>

// Other includes
#include "structs.h"
#include "logging.h"
#include "SimulationManager.h"
#include "SimulationUtils.h"
#include "ControlTower.h"


//Global variables
int shmid, msqid;
shared_t *shm_struct;
pthread_t timer_thread, pipe_thread;
pid_t control_tower;
FILE *log_file;
sem_t *mutex_log;
queue_t *arrival_queue;
queue_t *departure_queue;


int main() {
    config_t configs;


    sem_unlink("LOG_MUTEX");
    mutex_log = sem_open("LOG_MUTEX", O_CREAT | O_EXCL, 0766, 1);
    if (mutex_log == (sem_t *) -1) {
        perror("Log file LOG_MUTEX creation failed");
        exit(0);
    }

    log_file = open_log(LOG_PATH, ON);
    log_status(log_file, STARTED, ON);

    configs = read_configs(CONFIG_PATH);


    log_debug(log_file, "Creating shared memory...", ON);
    if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_t), IPC_CREAT | 0777)) < 0) {
        log_error(log_file, "Shared memory allocation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE!", ON);


    log_debug(log_file, "Attaching shared memory...", ON);
    shm_struct = (shared_t *) shmat(shmid, NULL, 0);
    if (shm_struct == (shared_t *) -1) {
        log_error(log_file, "Shared memory attach failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE!", ON);

    shm_struct->time = 0;

    log_debug(log_file, "Creating Message Queue...", ON);
    if ((msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0777)) == -1) {
        log_error(log_file, "Message Queue creation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE!", ON);

    log_debug(log_file, "Creating Control Tower process...", ON);
    if ((control_tower = fork()) == 0) {
        log_debug(log_file, "Control Tower Active...", ON);
        tower_manager();
        exit(0);
    }
    log_debug(log_file, "DONE!", ON);

    log_debug(log_file, "Creating time Thread...", ON);
    if (pthread_create(&timer_thread, NULL, timer, (void *) (&configs.time_units))) {
        log_error(log_file, "Timer thread creation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE!", ON);

    log_debug(log_file, "Unlinking the named pipe...", ON);
    unlink(PIPE_NAME);
    if (mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0777) < 0) {
        log_error(log_file, "Named Pipe creation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE!", ON);

    log_debug(log_file, "Creating flight waiting queue", ON);
    arrival_queue = create_queue(ARRIVAL_FLIGHT);
    departure_queue = create_queue(DEPARTURE_FLIGHT);

    if (!arrival_queue || !departure_queue) {
        log_error(log_file, "Flight queue creation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE!", ON);

    //TODO: need thread to check if init == current_time

    log_debug(log_file, "Starting Simulation...", ON);

    log_debug(log_file, "Creating pipe reader Thread...", ON);
    if (pthread_create(&pipe_thread, NULL, pipe_reader, NULL)) {
        log_error(log_file, "Pipe reader thread creation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE!", ON);


    signal(SIGINT, end_program);

    pthread_join(timer_thread, NULL);
    pthread_join(pipe_thread, NULL);

    exit(0);
}