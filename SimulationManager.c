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

// Other includes
#include "structs.h"
#include "logging.h"
#include "SimulationManager.h"
#include "SimulationUtils.h"

//Global variables
int shmid, msqid;
shared_t *shm_struct;
pthread_t timer_thread;
FILE *log_file;

int main() {
    int time_control = 0;
    int words, fd, n_read;
    char buffer[BUF_SIZE], save[BUF_SIZE], *token;

    //signal(SIGINT, end_program);

    pid_t control_tower;
    flight_t *booker;
    queue_t *queue;

    config_t configs = read_configs(CONFIG_PATH);

    clean_log();

    log_file = open_log(LOG_PATH);
    log_status(log_file, STARTED, ON);


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


    log_debug(log_file, "Creating time Thread...", ON);
    if (pthread_create(&timer_thread, NULL, timer, (void *) (&configs.time_units))) {
        log_error(log_file, "Timer thread creation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE!", ON);


    log_debug(log_file, "Creating Message Queue...", ON);
    if ((msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0777)) == -1) {
        log_error(log_file, "Message Queue creation failed", ON);
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
    queue = create_queue();
    if (!queue) {
        log_error(log_file, "Flight queue creation failed", ON);
        exit(0);
    }
    log_debug(log_file, "DONE!", ON);

    //TODO: need thread for check if init == current_time

    log_debug(log_file, "Creating Control Tower process...", ON);
    if ((control_tower = fork()) == 0) {
        log_debug(log_file, "Control Tower Active...", ON);
        //tower_manager();
        exit(0);
    }
    log_debug(log_file, "DONE!", ON);

    log_debug(log_file, "Starting Simulation...", ON);

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

        log_info(log_file, "RECEIVED => ", buffer, ON);

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