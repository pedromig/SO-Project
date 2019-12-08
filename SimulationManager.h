/*
 *      SimulationManager.c
 *
 *      Copyright 2019 Miguel Rabuge
 *      Copyright 2019 Pedro Rodrigues
 */

#ifndef SIMULATION_MANAGER_H
#define SIMULATION_MANAGER_H

// Pipe name definition
#define PIPE_NAME "input_pipe"

// Configuration file name definition
#define CONFIG_PATH "config.txt"

// Standard buffer size used across the program
#define BUF_SIZE 1024

// C standard library includes
#include <pthread.h>

// Other includes
#include "structs.h"


// Global variables
extern int shmid, msqid, fd,num_flights;
extern pthread_t timer_thread, pipe_thread, arrivals_handler, departures_handler;
extern pthread_condattr_t shareable_cond;
extern pid_t control_tower;
extern FILE *log_file;
extern shared_t *shm_struct;
extern sem_t *mutex_log, *tower_mutex, *shm_mutex, *runway_mutex;
extern pthread_mutex_t mutex_arrivals, mutex_departures, listener_mutex;
extern queue_t *arrival_queue;
extern queue_t *departure_queue;
extern pthread_t *flight_threads;
extern config_t configs;

#endif //SIMULATION_MANAGER_H