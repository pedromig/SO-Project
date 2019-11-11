#ifndef SIMULATION_MANAGER_H
#define SIMULATION_MANAGER_H


#define PIPE_NAME "input_pipe"
#define CONFIG_PATH "config.txt"
#define BUF_SIZE 1024

#include <pthread.h>

#include "structs.h"

extern int shmid, msqid;
extern pthread_t timer_thread, pipe_thread, arrivals_handler, departures_handler;
extern pthread_cond_t time_refresher;
extern pid_t control_tower;
extern FILE *log_file;
extern shared_t *shm_struct;
extern sem_t *mutex_log, *tower_mutex;
extern pthread_mutex_t mutex_arrivals, mutex_departures;
extern queue_t *arrival_queue;
extern queue_t *departure_queue;
extern pthread_t *air_departures, *air_arrivals;

#endif //SIMULATION_MANAGER_H