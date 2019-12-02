#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>
#include <wait.h>

// Other includes
#include "structs.h"
#include "SimulationManager.h"
#include "SimulationUtils.h"
#include "logging.h"

#define LINE_BUF 30

pthread_mutex_t thread_array_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    pthread_mutex_lock(&mutex_arrivals);
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
    pthread_mutex_unlock(&mutex_arrivals);
}

void add_departure(queue_t *head, departure_t *flight) {
    pthread_mutex_lock(&mutex_departures);
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
    pthread_mutex_unlock(&mutex_departures);
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

void remove_flight(queue_t *head, int init, flight_t *structure) {
    queue_t *current;
    queue_t *before;

    find_flight(head, &before, &current, init);
    if (current != NULL) {
        memcpy(structure, &(current->flight), sizeof(current->flight));
        before->next = current->next;
        free(current);
    }
}

void print_queue(queue_t *head) {
    queue_t *current = head->next;
    char *type[2] = {"DEPARTURE", "ARRIVAL"};
    if (head->type == ARRIVAL_FLIGHT)
        printf("\n\t\t%s<-------- Booked Flights -------->%s\n", BLUE, RESET);
    else
        printf("\n\t\t%s<-------- Booked Flights -------->%s\n", RED, RESET);

    while (current) {
        if (current->type == ARRIVAL_FLIGHT) {
            printf("\n\t\t\t[%s name:%s]\n", type[ARRIVAL_FLIGHT], current->flight.a_flight->name);
            printf("\t\t\t>>> init:%d\n", current->flight.a_flight->init);
            printf("\t\t\t>>> eta:%d\n", current->flight.a_flight->eta);
            printf("\t\t\t>>> fuel:%d\n\n", current->flight.a_flight->fuel);
        } else {
            printf("\n\t\t\t[%s name:%s]\n", type[DEPARTURE_FLIGHT], current->flight.d_flight->name);
            printf("\t\t\t>>> init:%d\n", current->flight.d_flight->init);
            printf("\t\t\t>>> takeoff:%d\n\n", current->flight.d_flight->takeoff);
        }

        current = current->next;
    }
    if (head->type == ARRIVAL_FLIGHT)
        printf("\t\t%s<-------------------------------->%s\n\n", BLUE, RESET);
    else
        printf("\t\t%s<-------------------------------->%s\n\n", RED, RESET);


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

departure_t *load_departure_flight(char *name, const int *params, int current_time) {
    departure_t *new_flight = (departure_t *) malloc(sizeof(departure_t));

    if (new_flight && params[0] > current_time && params[1] > params[0]) {
        strcpy(new_flight->name, name);
        new_flight->init = params[0];
        new_flight->takeoff = params[1];
    } else {
        new_flight = NULL;
    }
    return new_flight;
}

departure_t *check_departure(char *buffer, int current_time) {
    char *name = NULL, *token, delimiter[2] = " ";
    char *dict[2] = {"init", "takeoff"};
    char string[strlen(buffer) + 1];
    int values[2];
    int i = 0, j = 0;
    int correct = 1;

    strcpy(string, buffer);

    token = strtok(string, delimiter);
    if (!strcmp(token, "DEPARTURE")) {
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

    return correct ? load_departure_flight(name, values, current_time) : NULL;
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
        log_debug(NULL, log_str, ON);                       // Disabled the writing of the time in the log file
        print_queue(departure_queue);                          // To much spam...
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
    int words, n_read;
    char buffer[BUF_SIZE];

    departure_t *d_booker;
    arrival_t *a_booker;
    if ((fd = open(PIPE_NAME, O_RDWR)) < 0) {
        log_error(log_file, "Error opening the pipe for reading...", ON);
        exit(0);
    }
    while (1) {
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
    }
}

//########################################  INIT QUEUE HANDLER ########################################################
void *arrivals_creation(void *nothing) {
    int time, found = 0;
    queue_t *current;
    flight_t *to_fly;
    int i;
    while (1) {

        pthread_mutex_lock(&mutex_arrivals);

        pthread_cond_wait(&time_refresher, &mutex_arrivals);
        current = arrival_queue->next;
        time = get_time();

        while (current && (current->flight.a_flight->init == time)) {
            to_fly = (flight_t *) malloc(sizeof(flight_t));
            remove_flight(arrival_queue, current->flight.a_flight->init, to_fly);

            pthread_mutex_lock(&thread_array_mutex);
            for (i = 0; i < num_flights && !found; i++) {
                if (flight_threads[i] == STATE_FREE) {
                    to_fly->a_flight->flight_id = i;
                    if (pthread_create(&flight_threads[i], NULL, arrival_execution, to_fly)) {
                        log_error(log_file, "Arrival thread creation failed", ON);
                        exit(0);
                    }
                    found = 1;
                }
            }
            pthread_mutex_unlock(&thread_array_mutex);
            found = 0;
            current = current->next;
        }
        pthread_mutex_unlock(&mutex_arrivals);
    }
}

void *departures_creation(void *nothing) {
    int time, found = 0;
    queue_t *current;
    flight_t *to_fly;
    int i;
    while (1) {

        pthread_mutex_lock(&mutex_departures);

        pthread_cond_wait(&time_refresher, &mutex_departures);
        current = departure_queue->next;
        time = get_time();

        while (current && (current->flight.d_flight->init == time)) {
            to_fly = (flight_t *) malloc(sizeof(flight_t));

            remove_flight(departure_queue, current->flight.d_flight->init, to_fly);

            pthread_mutex_lock(&thread_array_mutex);
            for (i = 0; i < num_flights && !found; i++) {
                if (flight_threads[i] == STATE_FREE) {
                    to_fly->d_flight->flight_id = i;
                    if (pthread_create(&flight_threads[i], NULL, departure_execution, to_fly)) {
                        log_error(log_file, "Departure thread creation failed", ON);
                        exit(0);
                    }
                    found = 1;
                }
            }
            pthread_mutex_unlock(&thread_array_mutex);
            found = 0;
            current = current->next;
        }
        pthread_mutex_unlock(&mutex_departures);
    }
}

//########################################  ARRIVAL/DEPARTURE EXECUTION HANDLERS ##################################
void *departure_execution(void *flight_info) {
    int my_slot;
    char str[BUF_SIZE];
    flight_t flight = *((flight_t *) flight_info);
    msg_t departure_message, answer_msg;

    sprintf(str, "Ready to Fly! Departure_ID: %d", flight.d_flight->flight_id);
    log_info(log_file, str, ON);
    memset(&str, 0, sizeof(str));

    //message setup
    departure_message.msg_type = (long) FLIGHT_THREAD_REQUEST;
    departure_message.answer_msg_type = (long) flight.d_flight->flight_id + FLIGHT_THREAD_REQUEST + 1;
    departure_message.takeoff = flight.d_flight->takeoff;
    departure_message.fuel = NOT_APLICABLE;
    departure_message.eta NOT_APLICABLE;
    //message queue functions

    if (msgsnd(msqid, &departure_message, sizeof(departure_message) - sizeof(long), IPC_NOWAIT)) {
        log_error(log_file, "Failed to send message to the Control Tower", ON);
    }

    if (msgrcv(msqid, &answer_msg, sizeof(answer_msg), departure_message.answer_msg_type, 0) < 0) {
        log_error(log_file, "Thread: Failed to receive message from the flight!", ON);
    }

    sprintf(str, "Departure flight [%d] --> Shared Memory Slot: %d", flight.d_flight->flight_id, answer_msg.slot);
    log_info(log_file, str, ON);

    free(flight_info);
    // TODO: Signal someone to join the thread and put the slot in the thread array back to STATE_FREE
    pthread_cancel(pthread_self());
    return NULL;
}

void *arrival_execution(void *flight_info) {
    char str[BUF_SIZE];
    flight_t flight = *((flight_t *) flight_info);
    msg_t arrival_message, answer_msg;

    sprintf(str, "Ready to land! Arrival_ID: %d", flight.a_flight->flight_id);
    log_info(log_file, str, ON);
    memset(&str, 0, sizeof(str));

    //message setup
    arrival_message.msg_type = (long) FLIGHT_THREAD_REQUEST;
    arrival_message.answer_msg_type = flight.a_flight->flight_id + FLIGHT_THREAD_REQUEST + 1;
    arrival_message.takeoff = NOT_APLICABLE;
    arrival_message.fuel = flight.a_flight->fuel;
    arrival_message.eta = flight.a_flight->eta;
    //message queue functions

    if (msgsnd(msqid, &arrival_message, sizeof(arrival_message) - sizeof(long), IPC_NOWAIT)) {
        log_error(log_file, "Failed to send message to the Control Tower", ON);
    }

    if (msgrcv(msqid, &answer_msg, sizeof(answer_msg), arrival_message.answer_msg_type, 0) < 0) {
        log_error(log_file, "Thread: Failed to receive message from the flight!", ON);
    }

    sprintf(str, "Arrival flight [%d] --> Shared Memory Slot: %d", flight.d_flight->flight_id, answer_msg.slot);
    log_info(log_file, str, ON);

    free(flight_info);
    // TODO: Signal someone to join the thread and put the slot in the thread array back to STATE_FREE
    pthread_cancel(pthread_self());
    return NULL;
}

//########################################  SIGNAL HANDLER ########################################################
void end_program(int signo) {

    printf("\n");
    log_info(log_file, "Simulation Manager: Received SIGINT!", ON);

    log_debug(log_file, "Canceling and joining all threads...", ON);
    pthread_cancel(timer_thread);
    pthread_cancel(pipe_thread);

    for (int i = 0; i < num_flights; i++) {
        if (flight_threads[i] != STATE_FREE) {
            if (pthread_join(flight_threads[i], NULL))
                log_error(log_file, "Failed to join flight thread!\n", ON);
        }
    }

    pthread_cancel(departures_handler);
    pthread_cancel(arrivals_handler);

    pthread_join(timer_thread, NULL);
    pthread_join(pipe_thread, NULL);
    pthread_join(departures_handler, NULL);
    pthread_join(arrivals_handler, NULL);
    log_debug(log_file, "DONE! (All Simulation Manager threads joined!)", ON);

    log_debug(log_file, "Killing Control Tower process...", ON);
    kill(control_tower, SIGUSR2);
    while (wait(NULL) > 0);
    log_debug(log_file, "DONE! (Control Tower process terminated!)", ON);

    log_debug(log_file, "Deleting Shared Memory...", ON);
    shmdt(shm_struct);
    shmctl(shmid, IPC_RMID, NULL);
    log_debug(log_file, "DONE! (Shared Memory deleted!)", ON);

    log_debug(log_file, "Deleting Message Queue...", ON);
    msgctl(msqid, IPC_RMID, NULL);
    log_debug(log_file, "DONE! (Message Queue Deleted!)", ON);


    log_debug(log_file, "Freeing thread id array...", ON);
    free(flight_threads);
    log_debug(log_file, "DONE! (Flight thread id array freed!)", ON);

    log_debug(log_file, "Deleting flight queues...", ON);
    delete_queue(arrival_queue);
    delete_queue(departure_queue);
    log_debug(log_file, "DONE! (Flight queues deleted!)", ON);


    pthread_mutex_destroy(&thread_array_mutex);
    pthread_mutex_destroy(&mutex_departures);
    pthread_cond_destroy(&time_refresher);

    log_debug(log_file, "Unlinking and deleting wait tower semaphore...", ON);
    sem_unlink("WAIT_TOWER");
    sem_close(tower_mutex);
    log_debug(log_file, "DONE! (Wait Tower semaphore unlinked and deleted!)", ON);

    log_debug(log_file, "Closing log file, unlinking pipe and deleting the semaphores!...", ON);
    log_debug(log_file, "DONE! (Closing file, unlinking the named pipe and deleting semaphores !)", OFF);
    log_status(log_file, CONCLUDED, OFF);

    close(fd);
    unlink(PIPE_NAME);
    fclose(log_file);

    log_debug(NULL, "DONE! (Closing file, unlinking the named pipe and deleting semaphores!)", ON);
    log_status(NULL, CONCLUDED, ON);

    sem_unlink("LOG_MUTEX");
    sem_close(mutex_log);

    exit(0);
}
//(Unlinking and deleting semaphores and closing log file!)