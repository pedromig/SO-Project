
#ifndef STRUCTS_H
#define STRUCTS_H

#define DEPARTURE_FLIGHT 0
#define ARRIVAL_FLIGHT 1
#define BUF_SIZE 1024

// Flight Structure
typedef struct Flight {
    char name[BUF_SIZE];
    int init_time;
    int takeoff_time;
    int eta;
    int fuel;
    int type;
} flight_t;

// Structure shared in memory
typedef struct Shared {
    // TODO: Statistics
    int time;
} shared_t;

// Struct containing the configurations
typedef struct Configurations {
    int time_units;

    int takeoff_time;
    int takeoff_gap;

    int landing_time;
    int landing_gap;

    int holding_min;
    int holding_max;

    int max_departures;
    int max_arrivals;

} config_t;

typedef struct Queue {
    flight_t *flight;
    struct Queue *next;
}queue_t;

#endif // STRUCTS_H