#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>

// Other includes
#include "structs.h"
#include "SimulationManager.h"
#include "logging.h"

#define LINE_BUF 30

//######################################## CONFIGURATIONS ###################################################

config_t load_struct(const int *array) {
    config_t config;

    config.time_units = array[0];
    config.takeoff_time = array[1];
    config.takeoff_gap = array[2];
    config.landing_time = array[3];
    config.landing_gap = array[4];
    config.holding_min = array[5];
    config.holding_max = array[6];
    config.max_departures = array[7];
    config.max_arrivals = array[8];

    return config;
}

config_t read_configs(char *fname) {
    int i = 0;
    int array[9];
    char line[LINE_BUF], *token, delimiter[3] = ", ";

    FILE *fp = fopen(fname, "r");

    if (!fp) {
        perror("Failed to open configuration file!");
        exit(0);
    }

    while (fgets(line, LINE_BUF, fp) != NULL) {
        token = strtok(line, delimiter);
        while (token != NULL) {
            if (i > 9) {
                printf("Error reading file!\n");
                exit(0);
            }
            array[i++] = atoi(token);
            token = strtok(NULL, delimiter);
        }
    }

    return load_struct(array);
}

//######################################## FLIGHT LIST/QUEUE #####################################################

queue_t *create_queue() {
    queue_t *queue = (queue_t *) malloc(sizeof(queue_t));
    if (queue) {
        queue->flight = NULL;
        queue->next = NULL;
    }
    return queue;
}

void add_queue(queue_t *head, flight_t *flight) {
    queue_t *last = head;
    queue_t *current = head->next;
    queue_t *new;
    while (current && (current->flight->init_time < flight->init_time)) {
        current = current->next;
        last = last->next;
    }
    new = malloc(sizeof(queue_t));
    new->flight = flight;
    new->next = current;
    last->next = new;
}

void print_queue(queue_t *head) {
    queue_t *current = head->next;
    char *type[2];
    type[0] = "DEPARTURE";
    type[1] = "ARRIVAL";
    printf("Booked Flights:\n");
    while (current) {
        printf("[%s init:%d]\n", type[current->flight->type], current->flight->init_time);
        current = current->next;
    }
}

//############################################ COMMAND PARSING #################################################

int isnumber(char *string) {
    int status = 1;
    int i, len = (int) strlen(string);
    for (i = 0; i < len && status != 0; i++)
        if (!isdigit(string[i]))
            status = 0;
    return status;
}

flight_t *load_flight_struct(char *name, const int *params, int current_time) {
    flight_t *new_flight = (flight_t *) malloc(sizeof(flight_t));

    if (new_flight && params[0] >= current_time && params[1] <= params[2]) {
        strcpy(new_flight->name, name);
        new_flight->init_time = params[0];
        new_flight->eta = params[1];
        new_flight->fuel = params[2];
        new_flight->type = ARRIVAL_FLIGHT;
        new_flight->takeoff_time = -1;
    } else {
        new_flight = NULL;
    }
    return new_flight;
}

flight_t *check_arrival(char *buffer, int current_time) {
    char *name, *token, delimiter[2] = " ";
    char *dict[3] = {"init", "eta", "fuel"};
    int values[3];
    int i = 0, j = 0;
    int correct = 1;


    token = strtok(buffer, delimiter);
    if (!strcmp(token, "ARRIVAL")) {
        name = strtok(NULL, delimiter);
    } else {
        correct = 0;
    }

    token = strtok(NULL, delimiter);
    while (token && correct) {
        if ((i % 2 == 0 && strcmp(token, dict[j]) == 1)) {
            correct = 0;
        } else if (i % 2 != 0 && isnumber(token)) {
            values[j] = atoi(token);
            j++;
        }
        i++;
        token = strtok(NULL, delimiter);
    }

    return correct ? load_flight_struct(name, values, current_time) : NULL;
}

flight_t *check_departure(char *buffer, int current_time) {
    char *aux;
    char *keyword;
    char *flight_code;
    int init, takeoff;
    flight_t *flight;
    //DEPARTURE
    keyword = strtok(buffer, " ");
    if (strcmp(keyword, "DEPARTURE")) {
        return NULL;
    }
    //flight_code
    flight_code = strtok(NULL, " ");
    //init:
    aux = strtok(NULL, " ");
    if (strcmp(aux, "init:")) {
        return NULL;
    }
    // init value
    aux = strtok(NULL, " ");
    if (!isnumber(aux)) {
        return NULL;
    } else {
        init = atoi(aux);
        if (init < current_time) {
            return NULL;
        }
    }
    //takeoff:
    aux = strtok(NULL, " ");
    if (strcmp(aux, "takeoff:")) {
        return NULL;
    }
    //takeoff value
    aux = strtok(NULL, " ");
    if (!isnumber(aux)) {
        return NULL;
    } else {
        takeoff = atoi(aux);
        if ((takeoff < current_time) || (takeoff <= init)) {
            return NULL;
        }
    }
    flight = malloc(sizeof(flight_t));

    flight->type = DEPARTURE_FLIGHT;
    strcpy(flight->name, flight_code);
    flight->init_time = init;
    flight->takeoff_time = takeoff;
    flight->eta = -1;
    flight->fuel = -1;

    return flight;
}

//######################################## FLIGHT TIMER THREAD #####################################################

void *timer(void *time_units) {
    int time = *((int *) time_units);
    time = time * 1000;
    char log_str[BUF_SIZE];

    while (1) {
        shm_struct->time++;
        usleep(time);

        sprintf(log_str, "Time: %d", shm_struct->time);
        log_debug(log_file, log_str, ON);
    }
}

int get_time(void) {
    return shm_struct->time;
}
//########################################  PIPE THREAD ########################################################
void * pipe_reader(void* param_queue){
    int time_control = 0;
    int words, fd, n_read;
    char buffer[BUF_SIZE], save[BUF_SIZE], *token;
    flight_t *booker;
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
            if ((booker = check_departure(save, time_control))) {

                add_queue(queue, booker);
                printf("Departure queued!\n");
                //log_departure(log_file, booker->name,)

                print_queue(queue);
            } else {
                printf("Failed to book Flight!\n");
            }
        } else if (words == 8) {//ARRIVAL
            if ((booker = check_arrival(save, time_control))) {
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
//########################################  SIGNAL HANDLER ########################################################

void end_program(int signo) {
    signal(SIGINT, SIG_IGN);
    kill(control_tower,SIGKILL); // mata filho
    pthread_cancel(timer_thread); //cancela timer
    pthread_cancel(pipe_thread); //cancela pipe_reader
    shmctl(shmid, IPC_RMID, NULL);// remove shm
    msgctl(msqid, IPC_RMID, NULL);//remove msq
    //needed: remove linked list
    log_status(log_file, CONCLUDED, ON);//print consola e no log que terminou
    fclose(log_file);// fecha o log
    exit(0);// exit do processo pai
    //!!!!ver main do simulation manager(final) !!!!
}
    
    
    