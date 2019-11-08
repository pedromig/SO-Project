#ifndef SO_PROJECT_SIMULATIONUTILS_H
#define SO_PROJECT_SIMULATIONUTILS_H

void *timer(void *time_units);

int get_time(void);

flight_t *check_departure(char *buffer, int current_time);

flight_t *check_arrival(char *buffer, int current_time);

void add_queue(queue_t *head, flight_t *flight);

void print_queue(queue_t *head);

queue_t *create_queue();

config_t read_configs(char *fname);

void * pipe_reader(void* nothing);

void end_program(int signo);

#endif //SO_PROJECT_SIMULATIONUTILS_H
