/*
 *      SimulationManager.c
 *
 *      Copyright 2019 Miguel Rabuge Nº 2018293728
 *      Copyright 2019 Pedro Rodrigues Nº 2018283166
 */


// C standard library includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>

// Other includes
#include "structs.h"
#include "logging.h"
#include "SimulationManager.h"
#include "SimulationUtils.h"
#include "ControlTower.h"


// Global variables
int shmid, fd, msqid, num_flights;
shared_t *shm_struct;
pthread_t timer_thread, pipe_thread, arrivals_handler, departures_handler;
pthread_condattr_t shareable_cond;
pid_t control_tower;
FILE *log_file;
sem_t *mutex_log, *tower_mutex, *shm_mutex, *runway_mutex;
pthread_mutex_t mutex_arrivals = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_departures = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t listener_mutex = PTHREAD_MUTEX_INITIALIZER;
queue_t *arrival_queue;
queue_t *departure_queue;
pthread_t *flight_threads;
config_t configs;

int main() {
    sigset_t all_signals;

    // Exclude all signals but SIGINT (process-wide)
    sigfillset(&all_signals);
    sigdelset(&all_signals, SIGINT);
    sigprocmask(SIG_SETMASK, &all_signals, NULL);

    // Create log mutex
    sem_unlink("LOG_MUTEX");
    mutex_log = sem_open("LOG_MUTEX", O_CREAT | O_EXCL, 0766, 1);
    if (mutex_log == (sem_t *) -1) {
        perror("Log file LOG_MUTEX creation failed");
        exit(0);
    }

    // Create "wait for tower" mutex (tower must be generated before the simulator truly starts)
    sem_unlink("WAIT_TOWER");
    tower_mutex = sem_open("WAIT_TOWER", O_CREAT | O_EXCL, 0766, 1);
    if (tower_mutex == (sem_t *) -1) {
        perror("Tower semaphore creation failed");
        exit(0);
    }

    // Create shared memory slots mutex
    sem_unlink("RUNWAY_LOCKER");
    runway_mutex = sem_open("RUNWAY_LOCKER", O_CREAT | O_EXCL, 0766, 1);
    if (runway_mutex == (sem_t *) -1) {
        perror("Runway locker creation failed");
        exit(0);
    }
    // Create shared memory slots mutex
    sem_unlink("SHARED_MUTEX");
    shm_mutex = sem_open("SHARED_MUTEX", O_CREAT | O_EXCL, 0766, 1);
    if (shm_mutex == (sem_t *) -1) {
        perror("Slots shm mutex creation failed");
        exit(0);
    }

    // Open log file
    log_file = open_log(LOG_PATH, ON);

    // Read configurations from file
    configs = read_configs(CONFIG_PATH);
    num_flights = configs.max_arrivals + configs.max_departures;

    // Flight threads and thread numeric ids
    flight_threads = (pthread_t *) malloc(sizeof(pthread_t) * num_flights);
    memset(flight_threads, STATE_FREE, sizeof(pthread_t) * num_flights);

    // Shared Memory creation
    log_debug(NULL, "Creating shared memory... ", ON);
    if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_t) + (sizeof(int) * num_flights), IPC_CREAT | 0777)) < 0) {
        log_error(NULL, "Shared memory allocation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Shared Memory Created!)", ON);


    // Attaching shared memory
    log_debug(NULL, "Attaching shared memory...", ON);
    shm_struct = (shared_t *) shmat(shmid, NULL, 0);
    if (shm_struct == (shared_t *) -1) {
        log_error(NULL, "Shared memory attach failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Shared memory Attached!)", ON);

    // Creation Process-Wide condition variable attribute to allow inter-process condition signaling
    pthread_condattr_init(&shareable_cond);
    pthread_condattr_setpshared(&shareable_cond, PTHREAD_PROCESS_SHARED);

    // Condition variables creation with process shared behaviour
    pthread_cond_init(&(shm_struct->time_refresher), &shareable_cond);
    pthread_cond_init(&(shm_struct->listener), &shareable_cond);


    // Program time and thread ids array initialization
    shm_struct->time = 0;
    memset(&(shm_struct->flight_ids), STATE_FREE, num_flights * sizeof(int));

    // Message Queue Creation
    log_debug(NULL, "Creating Message Queue...", ON);
    if ((msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0777)) == -1) {
        log_error(NULL, "Message Queue creation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Message Queue created!)", ON);


    // Control Tower child process creation
    log_debug(NULL, "Creating Control Tower process...", ON);
    sem_wait(tower_mutex);
    if ((control_tower = fork()) == 0) {
        char controlTower[BUF_SIZE];
        sigset_t child_mask;

        // Block all signals but SIGUSR1 and SIGUSR2
        sigemptyset(&child_mask);
        sigaddset(&child_mask, SIGUSR1);
        sigaddset(&child_mask, SIGUSR2);
        sigprocmask(SIG_UNBLOCK, &child_mask, NULL);

        // Ignore SIGINT on the child process
        signal(SIGINT, SIG_IGN);

        sprintf(controlTower, "%sCONTROL TOWER PID:%s %d", BLUE, RESET, getpid());
        log_info(NULL, controlTower, ON);

        log_debug(NULL, "Control Tower Active...", ON);
        tower_manager();
        exit(0);
    }
    log_debug(NULL, "DONE!(Control Tower process!)", ON);


    // FIFO pipe creation
    log_debug(NULL, "Unlinking the named pipe...", ON);
    unlink(PIPE_NAME);
    if (mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0777) < 0) {
        log_error(NULL, "Named Pipe creation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Named Pipe created!)", ON);


    // Flight arrival and departure queues creation
    log_debug(NULL, "Creating flight waiting queue", ON);
    arrival_queue = create_queue(ARRIVAL_FLIGHT);
    departure_queue = create_queue(DEPARTURE_FLIGHT);
    if (!arrival_queue || !departure_queue) {
        log_error(NULL, "Flight queue creation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Flight Waiting Queue created!)", ON);


    // Arrival flight handler thread creation
    log_debug(NULL, "Creating arrivals handler Thread...", ON);
    if (pthread_create(&arrivals_handler, NULL, arrivals_creation, NULL)) {
        log_error(NULL, "Arrivals handler thread creation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Arrivals Handler thread created!)", ON);


    // Departure flight handler thread creation
    log_debug(NULL, "Creating departures handler Thread...", ON);
    if (pthread_create(&departures_handler, NULL, departures_creation, NULL)) {
        log_error(NULL, "Departures handler thread creation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Departures Handler thread created!)", ON);


    // Pipe reader thread creation
    log_debug(NULL, "Creating pipe reader Thread...", ON);
    if (pthread_create(&pipe_thread, NULL, pipe_reader, NULL)) {
        log_error(NULL, "Pipe reader thread creation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Pipe reader thread created!)", ON);

    // Wait for the control tower process creation
    sem_wait(tower_mutex);

    // Creating timer thread and starting the simulation
    log_debug(NULL, "Creating time Thread...", ON);
    if (pthread_create(&timer_thread, NULL, timer, (void *) (&configs.time_units))) {
        log_error(NULL, "Timer thread creation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Timer thread created!)", ON);

    log_info(NULL, "Starting Simulation...", ON);
    log_status(log_file, STARTED, ON);

    // Wait for SIGINT for program shutdown and structure cleanup
    signal(SIGINT, end_program);
    pause();
    exit(0);
}