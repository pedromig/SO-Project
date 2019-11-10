
#ifndef STRUCTS_H
#define STRUCTS_H

#define DEPARTURE_FLIGHT 0
#define ARRIVAL_FLIGHT 1
#define BUF_SIZE 1024


// Flight Structure
typedef struct ArrivalFlight {
    char name[BUF_SIZE];
    int init;
    int eta;
    int fuel;
} arrival_t;

typedef struct DepartureFlight {
    char name[BUF_SIZE];
    int init;
    int takeoff;
} departure_t;


typedef union Flight {
    arrival_t *a_flight;
    departure_t *d_flight;
} flight_t;

typedef struct Queue {
    flight_t flight;
    struct Queue *next;
    int type;
} queue_t;

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

#endif // STRUCTS_H