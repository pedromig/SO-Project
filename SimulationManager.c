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
    
    //read configurations from file
    configs = read_configs(CONFIG_PATH);
    num_flights = configs.max_arrivals + configs.max_departures;

    //flight threads and thread numeric ids
    flight_threads = (pthread_t *) malloc(sizeof(pthread_t) * num_flights);
    memset(flight_threads, STATE_FREE, sizeof(pthread_t) * num_flights);

    //shm creation
    log_debug(NULL, "Creating shared memory... ", ON);
    if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_t) + (sizeof(int) * num_flights), IPC_CREAT | 0777)) < 0) {
        log_error(NULL, "Shared memory allocation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Shared Memory Created!)", ON);


    //attaching shared memory
    log_debug(NULL, "Attaching shared memory...", ON);
    shm_struct = (shared_t *) shmat(shmid, NULL, 0);
    if (shm_struct == (shared_t *) -1) {
        log_error(NULL, "Shared memory attach failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Shared memory Attached!)", ON);


    //initializing time and flight_ids array
    shm_struct->time = 0;
    memset(&(shm_struct->flight_ids), STATE_FREE, num_flights * sizeof(int));

    //creating msq
    log_debug(NULL, "Creating Message Queue...", ON);
    if ((msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0777)) == -1) {
        log_error(NULL, "Message Queue creation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Message Queue created!)", ON);


    //tower manager
    log_debug(NULL, "Creating Control Tower process...", ON);
    if ((control_tower = fork()) == 0) {
        sem_wait(tower_mutex);
        signal(SIGINT, SIG_IGN);
        log_debug(NULL, "Control Tower Active...", ON);
        tower_manager();
        exit(0);
    }
    log_debug(NULL, "DONE!(Control Tower process!)", ON);


    //pipe creation
    log_debug(NULL, "Unlinking the named pipe...", ON);
    unlink(PIPE_NAME);
    if (mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0777) < 0) {
        log_error(NULL, "Named Pipe creation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Named Pipe created!)", ON);


    //init queues
    log_debug(NULL, "Creating flight waiting queue", ON);
    arrival_queue = create_queue(ARRIVAL_FLIGHT);
    departure_queue = create_queue(DEPARTURE_FLIGHT);
    if (!arrival_queue || !departure_queue) {
        log_error(NULL, "Flight queue creation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Flight Waiting Queue created!)", ON);


    // arrivals handler thread
    log_debug(NULL, "Creating arrivals handler Thread...", ON);
    if (pthread_create(&arrivals_handler, NULL, arrivals_creation, NULL)) {
        log_error(NULL, "Arrivals handler thread creation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Arrivals Handler thread created!)", ON);


    // departures handler thread
    log_debug(NULL, "Creating departures handler Thread...", ON);
    if (pthread_create(&departures_handler, NULL, departures_creation, NULL)) {
        log_error(NULL, "Departures handler thread creation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Departures Handler thread created!)", ON);


    //pipe reader thread
    log_debug(NULL, "Creating pipe reader Thread...", ON);
    if (pthread_create(&pipe_thread, NULL, pipe_reader, NULL)) {
        log_error(NULL, "Pipe reader thread creation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Pipe reader thread created!)", ON);

    //waiting for the tower process to be created
    sem_wait(tower_mutex);

    //time thread
    log_debug(NULL, "Creating time Thread...", ON);
    if (pthread_create(&timer_thread, NULL, timer, (void *) (&configs.time_units))) {
        log_error(NULL, "Timer thread creation failed", ON);
        exit(0);
    }
    log_debug(NULL, "DONE! (Timer thread created!)", ON);

    log_info(NULL, "Starting Simulation...", ON);
    log_status(log_file, STARTED, ON);

    //shutdown things
    signal(SIGINT,end_program);
    pause();

    exit(0);
}