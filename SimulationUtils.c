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
#include <semaphore.h>

// Other includes
#include "structs.h"
#include "SimulationManager.h"
#include "SimulationUtils.h"
#include "logging.h"

#define LINE_BUF 30


int isnumber(char *string) {
    int status = 1;
    int i, len = (int) strlen(string);
    for (i = 0; i < len && status != 0; i++)
        if (!isdigit(string[i]))
            status = 0;
    return status;
}

int wordCount(char *string) {
    int words = 0;
    char str[strlen(string) + 1];
    char *token, delimiter[2] = " ";

    strcpy(str, string);
    token = strtok(str, delimiter);
    while (token) {
        words++;
        token = strtok(NULL, delimiter);
    }
    return words;
}

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

    log_debug(log_file, "Opening Configurations file...", ON);
    FILE *fp = fopen(fname, "r");

    if (!fp) {
        log_error(log_file, "Failed to open configuration file!", ON);
        exit(0);
    }
    log_debug(log_file, "DONE!", ON);

    while (fgets(line, LINE_BUF, fp) != NULL) {
        token = strtok(line, delimiter);
        while (token != NULL) {
            if (i > 9) {
                log_debug(log_file, "Error occurred while reading the file!", ON);
                exit(0);
            }
            array[i++] = atoi(token);
            token = strtok(NULL, delimiter);
        }
    }

    return load_struct(array);
}

//######################################## FLIGHT LIST/QUEUE #####################################################

queue_t *create_queue(int type) {
    queue_t *head = (queue_t *) malloc(sizeof(queue_t));
    if (head) {
        head->flight.a_flight = NULL;
        head->flight.d_flight = NULL;
        head->next = NULL;
        head->type = type;
    }
    return head;
}

void delete_queue(queue_t *head) {
    queue_t *current;
    while (head != NULL) {
        current = head;
        head = head->next;
        free(current);
    }
    free(head);
}

void add_arrival(queue_t *head, arrival_t *flight) {
    queue_t *last = head;
    queue_t *current = head->next;
    queue_t *new = (queue_t *) malloc(sizeof(queue_t));

    while (current && (current->flight.a_flight->init < flight->init)) {
        current = current->next;
        last = last->next;
    }
    new->flight.a_flight = flight;
    new->type = ARRIVAL_FLIGHT;

    new->next = current;
    last->next = new;
}

void add_departure(queue_t *head, departure_t *flight) {
    queue_t *last = head;
    queue_t *current = head->next;
    queue_t *new = (queue_t *) malloc(sizeof(queue_t));

    while (current && (current->flight.d_flight->init < flight->init)) {
        current = current->next;
        last = last->next;
    }
    new->flight.d_flight = flight;
    new->type = DEPARTURE_FLIGHT;

    new->next = current;
    last->next = new;
}

void find_flight(queue_t *head, queue_t **before, queue_t **current, int init) {
    *before = head;
    *current = head->next;
    if (head->type == ARRIVAL_FLIGHT) {
        while ((*current) && (*current)->flight.a_flight->init != init) {
            *before = *current;
            *current = (*current)->next;
        }
        if ((*current) && (*current)->flight.a_flight->init != init) {
            *current = NULL;
        }
    } else {
        while ((*current) && (*current)->flight.d_flight->init != init) {
            *before = *current;
            *current = (*current)->next;
        }
        if ((*current) && (*current)->flight.d_flight->init != init) {
            *current = NULL;
        }
    }
}

void remove_flight(queue_t *head, int init, flight_t* estrutura) {
    queue_t *current;
    queue_t *before;

    find_flight(head, &before, &current, init);
    if (current != NULL) {
        *estrutura = current->flight;
        before->next = current->next;
        free(current);
    }
}

void print_queue(queue_t *head) {
    queue_t *current = head->next;
    char *type[2] = {"DEPARTURE", "ARRIVAL"};
    printf("\n\t\t<-------- Booked Flights -------->\n");
    while (current) {
        if (current->type == ARRIVAL_FLIGHT)
            printf("\t\t\t[%s init:%d]\n", type[ARRIVAL_FLIGHT], current->flight.a_flight->init);
        else
            printf("\t\t\t[%s init:%d]\n", type[DEPARTURE_FLIGHT], current->flight.d_flight->init);
        current = current->next;
    }
    printf("\t\t<-------------------------------->\n\n");

}

//############################################ COMMAND PARSING #################################################

arrival_t *load_arrival_flight(char *name, const int *params, int current_time) {
    arrival_t *new_flight = (arrival_t *) malloc(sizeof(arrival_t));

    if (new_flight && params[0] > current_time && params[1] <= params[2]) {
        strcpy(new_flight->name, name);
        new_flight->init = params[0];
        new_flight->eta = params[1];
        new_flight->fuel = params[2];
    } else {
        new_flight = NULL;
    }
    return new_flight;
}

arrival_t *check_arrival(char *buffer, int current_time) {
    char *name = NULL, *token, delimiter[2] = " ";
    char *dict[3] = {"init", "eta", "fuel"};
    char string[strlen(buffer) + 1];
    int values[3];
    int i = 0, j = 0;
    int correct = 1;

    strcpy(string, buffer);

    token = strtok(string, delimiter);
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

    return correct ? load_arrival_flight(name, values, current_time) : NULL;
}

departure_t *check_departure(char *buffer, int current_time) {
    char *aux;
    char *keyword;
    char *flight_code;
    int init, takeoff;
    departure_t *flight;
    char string[strlen(buffer) + 1];
    strcpy(string, buffer);

    //DEPARTURE
    keyword = strtok(string, " ");
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
        if (init <= current_time) {
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
        if ((takeoff < current_time) || (takeoff <= init)) {//tirar ((takeoff < current_time) || )?
            return NULL;
        }
    }
    flight = malloc(sizeof(flight_t));

    strcpy(flight->name, flight_code);
    flight->init = init;
    flight->takeoff = takeoff;

    return flight;
}

//######################################## FLIGHT TIMER THREAD #####################################################

void *timer(void *time_units) {
    int time = *((int *) time_units);
    time = time * 1000;
    char log_str[BUF_SIZE];

    while (1) {
        usleep(time);
        shm_struct->time++;
        sprintf(log_str, "Time: %d", shm_struct->time);
        log_debug(log_file, log_str, ON);
        print_queue(departure_queue);
        print_queue(arrival_queue);
        pthread_cond_broadcast(&time_refresher);
    }
}

int get_time(void) {
    return shm_struct->time;
}

//########################################  PIPE THREAD ########################################################

void *pipe_reader(void *param_queue) {
    int time_control = 0;
    int words, fd, n_read;
    char buffer[BUF_SIZE];

    departure_t *d_booker;
    arrival_t *a_booker;

    while (1) {

        if ((fd = open(PIPE_NAME, O_RDONLY)) < 0) {
            log_error(log_file, "Error opening the pipe for reading...", ON);
            exit(0);
        }

        n_read = read(fd, buffer, BUF_SIZE);
        buffer[n_read - 1] = '\0';

        time_control = get_time();

        words = wordCount(buffer);


        if (words == 6) {  // DEPARTURE

            if ((d_booker = check_departure(buffer, time_control))) {

                add_departure(departure_queue, d_booker);
                log_command(log_file, buffer, NEW_COMMAND, ON);

                print_queue(departure_queue);
            } else {
                log_command(log_file, buffer, WRONG_COMMAND, ON);
            }

        } else if (words == 8) {  // ARRIVAL

            if ((a_booker = check_arrival(buffer, time_control))) {
                add_arrival(arrival_queue, a_booker);
                log_command(log_file, buffer, NEW_COMMAND, ON);

                print_queue(arrival_queue);
            } else {
                log_command(log_file, buffer, WRONG_COMMAND, ON);
            }
        } else {
            log_command(log_file, buffer, WRONG_COMMAND, ON);
        }
        close(fd);
    }
}
//########################################  INIT QUEUE HANDLER ########################################################
void* arrivals_creation(void* nothing){
    int time;
    queue_t *current;
    flight_t to_fly;
    int air_counter = 0;
    while(1){
        
        pthread_mutex_lock(&mutex_arrivals);

        pthread_cond_wait(&time_refresher,&mutex_arrivals);
        current = arrival_queue->next;
        time = get_time();
       
        while(current && (current->flight.a_flight->init == time)){
            remove_flight(arrival_queue,current->flight.a_flight->init,&to_fly);
            shm_struct->arrivals_id[air_counter] = air_counter;
            if(pthread_create(&air_arrivals[air_counter],NULL,arrival_execution,&(shm_struct->arrivals_id[air_counter]))){
                log_error(log_file, "Arrival thread creation failed", ON);
                exit(0);
            }
            //shm_struct->arrivals_id[air_counter] = 0;
            air_counter++;
            current = current->next;
        } 
        pthread_mutex_unlock(&mutex_arrivals);
    }
}

void* departures_creation(void* nothing){
    int time;
    queue_t *current;
    flight_t to_fly;
    int air_counter = 0;
    while(1){
        
        pthread_mutex_lock(&mutex_departures);

        pthread_cond_wait(&time_refresher,&mutex_departures);
        current = departure_queue->next;
        time = get_time();
       
        while(current && (current->flight.d_flight->init == time)){
            remove_flight(departure_queue,current->flight.d_flight->init,&to_fly);
            shm_struct->departures_id[air_counter] = air_counter;
            if(pthread_create(&air_departures[air_counter],NULL,departure_execution,&(shm_struct->departures_id[air_counter]))){
                log_error(log_file, "Departure thread creation failed", ON);
                exit(0);
            }
            //shm_struct->departures_id[air_counter] = 0;
            air_counter++;
            current = current->next;
        } 
        pthread_mutex_unlock(&mutex_departures);
    }
}
//########################################  ARRIVAL/DEPARTURE EXECUTION HANDLERS ##################################
void* departure_execution(void* departure_id){
    int id = *((int *) departure_id);
    printf("Ready to Fly! Departure_ID: %d\n",id);
    //msq ready msg
    pthread_cancel(pthread_self());
    pthread_join(pthread_self(),NULL);
}

void* arrival_execution(void* arrival_id){
    int id = *((int *) arrival_id);
    printf("Ready to land! Arrival_ID: %d\n",id);
    //msq ready msg
    pthread_cancel(pthread_self());
    pthread_join(pthread_self(),NULL);
}
//########################################  SIGNAL HANDLER ########################################################
void end_program(int signo) {
    signal(SIGINT, SIG_IGN);

    kill(control_tower, SIGKILL);

    pthread_cancel(timer_thread);
    pthread_cancel(pipe_thread);
    pthread_cancel(departures_handler);
    pthread_cancel(arrivals_handler);

    free(shm_struct->arrivals_id);
    free(shm_struct->departures_id);

    shmctl(shmid, IPC_RMID, NULL);
    msgctl(msqid, IPC_RMID, NULL);

    free(air_arrivals);
    free(air_departures);

    delete_queue(arrival_queue);
    delete_queue(departure_queue);

    log_status(log_file, CONCLUDED, ON);
    fclose(log_file);
    sem_unlink("WAIT_TOWER");
    sem_unlink("LOG_MUTEX");
    sem_close(tower_mutex);
    sem_close(mutex_log);

    exit(0);
}
    
    
    