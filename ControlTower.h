#ifndef CONTROL_TOWER_H
#define CONTROL_TOWER_H

#include "SimulationManager.h"
#include "structs.h"

#define NUM_THREADS 1

extern pthread_t talker;
extern queue_t *fly_departures_queue, *land_arrivals_queue;

void tower_manager();

void* msq_comunicator(void* nothing);

void stats_show(int signo);

void* flights_updater(void* nothing);

void cleanup(int signo);

#endif // CONTROL_TOWER_H