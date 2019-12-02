#ifndef CONTROL_TOWER_H
#define CONTROL_TOWER_H

#include "SimulationManager.h"

#define NUM_THREADS 1

void tower_manager();

void stats_show(int signo);

void cleanup(int signo);

#endif // CONTROL_TOWER_H