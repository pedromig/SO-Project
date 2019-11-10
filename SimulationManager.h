#ifndef SIMULATION_MANAGER_H
#define SIMULATION_MANAGER_H


#define PIPE_NAME "input_pipe"
#define CONFIG_PATH "config.txt"
#define BUF_SIZE 1024

#include "structs.h"

extern int shmid, msqid;
extern pthread_t timer_thread, pipe_thread;
extern pid_t control_tower;
extern FILE *log_file;
extern shared_t *shm_struct;
extern sem_t *mutex_log;
extern queue_t *arrival_queue;
extern queue_t *departure_queue;

#endif //SIMULATION_MANAGER_H