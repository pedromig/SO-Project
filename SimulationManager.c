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
#include <wait.h>

// Other includes
#include "structs.h"
#include "logging.h"
#include "SimulationManager.h"
#include "SimulationUtils.h"
#include "ControlTower.h"


//Global variables
int shmid, fd, msqid, num_flights;
shared_t *shm_struct;
pthread_t timer_thread, pipe_thread, arrivals_handler, departures_handler;
pthread_cond_t time_refresher = PTHREAD_COND_INITIALIZER;
pid_t control_tower;
FILE *log_file;
sem_t *mutex_log, *tower_mutex;
pthread_mutex_t mutex_arrivals = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_departures = PTHREAD_MUTEX_INITIALIZER;
queue_t *arrival_queue;
queue_t *departure_queue;
pthread_t *flight_threads;


int main() {
    config_t configs;

    //create log mutex
    sem_unlink("LOG_MUTEX");
    mutex_log = sem_open("LOG_MUTEX", O_CREAT | O_EXCL, 0766, 1);
    if (mutex_log == (sem_t *) -1) {
        perror("Log file LOG_MUTEX creation failed");
        exit(0);
    }


    //create "wait for tower" mutex (tower must be generated before the simulator truly starts)
    sem_unlink("WAIT_TOWER");
    tower_mutex = sem_open("WAIT_TOWER", O_CREAT | O_EXCL, 0766, 1);
    if (tower_mutex == (sem_t *) -1) {
        perror("Tower semaphore creation failed");
        exit(0);
    }

    //open log
    log_file = open_log(LOG_PATH, ON);
    log_status(log_file, STARTED, ON);

    //read configurations from file
    configs = read_configs(CONFIG_PATH);
    num_flights = configs.max_arrivals + configs.max_departures;

    //flight threads and thread numeric ids
    flight_threads = (pthread_t *) malloc(sizeof(pthread_t) * num_flights);
    memset(flight_threads, STATE_FREE, sizeof(pthread_t) * num_flights);

    //shm creation
    log_debug(log_file, "Creating shared memory...", ON);
    if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_t) + (sizeof(int) * num_flights), IPC_CREAT | 0777)) < 0) {
        log_error(log_file, "Shared memory allocation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE! (Shared Memory Created!)", ON);


    //attaching shared memory
    log_debug(log_file, "Attaching shared memory...", ON);
    shm_struct = (shared_t *) shmat(shmid, NULL, 0);
    if (shm_struct == (shared_t *) -1) {
        log_error(log_file, "Shared memory attach failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE! (Shared memory Attached!)", ON);


    //initializing time and flight_ids array
    shm_struct->time = 0;
    memset(&(shm_struct->flight_ids), STATE_FREE, num_flights * sizeof(int));

    //creating msq
    log_debug(log_file, "Creating Message Queue...", ON);
    if ((msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0777)) == -1) {
        log_error(log_file, "Message Queue creation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE! (Message Queue created!)", ON);


    //tower manager
    log_debug(log_file, "Creating Control Tower process...", ON);
    sem_wait(tower_mutex);
    if ((control_tower = fork()) == 0) {
        log_debug(log_file, "Control Tower Active...", ON);
        tower_manager();
        exit(0);
    }
    log_debug(log_file, "DONE!(Control Tower process!)", ON);


    //pipe creation
    log_debug(log_file, "Unlinking the named pipe...", ON);
    unlink(PIPE_NAME);
    if (mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0777) < 0) {
        log_error(log_file, "Named Pipe creation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE! (Named Pipe created!)", ON);


    //init queues
    log_debug(log_file, "Creating flight waiting queue", ON);
    arrival_queue = create_queue(ARRIVAL_FLIGHT);
    departure_queue = create_queue(DEPARTURE_FLIGHT);
    if (!arrival_queue || !departure_queue) {
        log_error(log_file, "Flight queue creation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE! (Flight Waiting Queue created!)", ON);


    // arrivals handler thread
    log_debug(log_file, "Creating arrivals handler Thread...", ON);
    if (pthread_create(&arrivals_handler, NULL, arrivals_creation, NULL)) {
        log_error(log_file, "Arrivals handler thread creation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE! (Arrivals Handler thread created!)", ON);


    // departures handler thread
    log_debug(log_file, "Creating departures handler Thread...", ON);
    if (pthread_create(&departures_handler, NULL, departures_creation, NULL)) {
        log_error(log_file, "Departures handler thread creation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE! (Departures Handler thread created!)", ON);


    //pipe reader thread
    log_debug(log_file, "Creating pipe reader Thread...", ON);
    if (pthread_create(&pipe_thread, NULL, pipe_reader, NULL)) {
        log_error(log_file, "Pipe reader thread creation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE! (Pipe reader thread created!)", ON);

    //waiting for the tower process to be created
    sem_wait(tower_mutex);

    //time thread
    log_debug(log_file, "Creating time Thread...", ON);
    if (pthread_create(&timer_thread, NULL, timer, (void *) (&configs.time_units))) {
        log_error(log_file, "Timer thread creation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE! (Timer thread created!)", ON);

    log_info(log_file, "Starting Simulation...", ON);


    //shutdown things
    signal(SIGINT, end_program);

    log_debug(log_file, "Waiting for process and threads to join...", ON);
    while (wait(NULL) < 0);

    exit(0);
}