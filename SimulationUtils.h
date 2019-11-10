#ifndef SO_PROJECT_SIMULATIONUTILS_H
#define SO_PROJECT_SIMULATIONUTILS_H

void *timer(void *time_units);

int get_time(void);

departure_t *check_departure(char *buffer, int current_time);

arrival_t *check_arrival(char *buffer, int current_time);

void add_arrival(queue_t *head, arrival_t *flight);

void add_departure(queue_t *head, departure_t *flight);

void print_queue(queue_t *head);

queue_t *create_queue(int type);

config_t read_configs(char *fname);

void *pipe_reader(void *nothing);

void end_program(int signo);

void delete_queue(queue_t *head);

void* arrivals_creation(void* nothing);

void* departures_creation(void* nothing);

void* departure_execution(void* departure_id);

void* arrival_execution(void* arrival_id);
  
#endif //SO_PROJECT_SIMULATIONUTILS_H
