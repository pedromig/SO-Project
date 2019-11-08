#ifndef SIMULATION_MANAGER_H
#define SIMULATION_MANAGER_H


#define PIPE_NAME "input_pipe"
#define CONFIG_PATH "config.txt"
#define BUF_SIZE 1024

extern int shmid, msqid;
extern shared_t *shm_struct;
extern pthread_t timer_thread, pipe_thread;
extern pid_t control_tower;
extern queue_t *queue;
extern FILE *log_file;

#endif //SIMULATION_MANAGER_H