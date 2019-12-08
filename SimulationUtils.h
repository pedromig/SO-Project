#ifndef SO_PROJECT_SIMULATIONUTILS_H
#define SO_PROJECT_SIMULATIONUTILS_H

#define STATE_FREE 0
#define STATE_OCCUPIED 1
#define JOIN_THREAD -1


extern pthread_mutex_t thread_array_mutex;

#include "structs.h"

void *timer(void *time_units);

int get_time(void);

departure_t *check_departure(char *buffer, int current_time);

arrival_t *check_arrival(char *buffer, int current_time);

void add_arrival(queue_t *head, arrival_t *flight);

void add_arrival_TC(queue_t *head, arrival_t *flight);

void add_departure(queue_t *head, departure_t *flight);

void print_queue(queue_t *head);

queue_t *create_queue(int type);

void remove_flight_TC(queue_t *head, int slot, flight_t *structure);

void find_flight_TC(queue_t *head, queue_t **before, queue_t **current, int slot);

void remove_flight(queue_t *head, int init, flight_t *structure);

config_t read_configs(char *fname);

void *pipe_reader(void *nothing);

void end_program(int signo);

void delete_queue(queue_t *head);

void *arrivals_creation(void *nothing);

void *departures_creation(void *nothing);

void *departure_execution(void *flight_info);

void *arrival_execution(void *flight_info);

#endif //SO_PROJECT_SIMULATIONUTILS_H
