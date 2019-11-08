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

// Other includes
#include "structs.h"
#include "logging.h"
#include "SimulationManager.h"
#include "SimulationUtils.h"

//Global variables
int shmid, msqid;
shared_t *shm_struct;
pthread_t timer_thread;


int main() {
    int time_control = 0;
    int words, fd, n_read;
    char buffer[BUF_SIZE], save[BUF_SIZE], *token;

    pid_t control_tower;
    flight_t *booker;
    queue_t *queue;

    config_t configs = read_configs(CONFIG_PATH);

    FILE *log_file = open_log(LOG_PATH);
    clean_log();
    log_status(log_file, STARTED);


    //shared memory
    if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_t), IPC_CREAT | 0777)) < 0) {
        printf("Erro: shmget!\n");
        exit(0);
    }
    shm_struct = (shared_t *) shmat(shmid, NULL, 0);
    if (shm_struct == (shared_t *) -1) {
        printf("Erro: shmat!\n");
        exit(0);
    }

    //start program timer
    shm_struct->time = 0;
    if (pthread_create(&timer_thread, NULL, timer, (void *) (&configs.time_units))) {
        printf("Timer initialization failed!\n");
        exit(0);
    }

    //message queue
    if ((msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0777)) == -1) {
        printf("Erro: msgget!\n");
        exit(0);
    }

    //named pipe
    unlink(PIPE_NAME);
    if (mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0777) < 0) {
        printf("Erro: Pipe!");
        exit(0);
    }

    //queue initialization
    queue = make_queue();

    //need thread for check if init == current_time
    //control_tower process creation
    if ((control_tower = fork()) == 0) {
        //tower_manager();
        printf("control tower activated!\n");
        exit(0);
    }

    //simulator
    while (1) {
        //wait for something in the input_pipe
        if ((fd = open(PIPE_NAME, O_RDONLY)) < 0) {
            printf("Cannot open pipe for reading!\n ");
            exit(0);
        }
        //when "open()" unblocks, read input_pipe into the buffer
        n_read = read(fd, buffer, BUF_SIZE);
        time_control = get_time();
        buffer[n_read - 1] = '\0';//terminate with \0
        strcpy(save, buffer);//saved copy of the buffer
        //print message received from pipe
        printf("RECEIVED => %s\n", buffer);
        //process the information
        token = strtok(buffer, " ");
        words = 0;
        while (token) {
            token = strtok(NULL, " ");
            words++;
        }
        if (words == 6) {//DEPARTURE
            if (booker = check_departure(save, time_control)) {

                add_queue(queue, booker);
                printf("Departure queued!\n");
                //log_departure(log_file, booker->name,)

                print_queue(queue);
            } else {
                printf("Failed to book Flight!\n");
            }
        } else if (words == 8) {//ARRIVAL
            if (booker = check_arrival(save, time_control)) {
                add_queue(queue, booker);
                printf("Arrival queued!\n");
                printf("    FLIGHT: %s\n    init: %d\n    eta: %d\n    fuel: %d\n", booker->name, booker->init_time,
                       booker->eta, booker->fuel);
                print_queue(queue);
            } else {
                printf("Failed to book Landing!\n");
            }
        } else {
            printf("Bad request!\n");
        }
        //close pipe file descriptor
        booker = NULL;
        close(fd);
    }

}