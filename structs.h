/*
 *      structs.h
 *
 *      Copyright 2019 Miguel Rabuge Nº 2018293728
 *      Copyright 2019 Pedro Rodrigues Nº 2018283166
 */

#ifndef STRUCTS_H
#define STRUCTS_H

// Flight type definitions
#define DEPARTURE_FLIGHT 0
#define ARRIVAL_FLIGHT 1

// Predefined message types
#define FLIGHT_THREAD_REQUEST 2
#define FLIGHT_PRIORITY_REQUEST 1
#define NOT_APLICABLE -1

// Flight maneuver commands
#define FLY_LAND 15
#define HOLDING 12
#define DETOUR 13
#define EMERGENCY 14

// A generic buffer size
#define BUF_SIZE 1024

// Arrival Flight Structure
typedef struct ArrivalFlight {
    char name[BUF_SIZE];
    int flight_id;
    int init;
    int eta;
    int fuel;
} arrival_t;

// Departure Flight Structure
typedef struct DepartureFlight {
    char name[BUF_SIZE];
    int flight_id;
    int init;
    int takeoff;
} departure_t;

// Union that represent a flight
typedef union Flight {
    arrival_t *a_flight;
    departure_t *d_flight;
} flight_t;

// Linked List / Queue structure
typedef struct Queue {
    int type;
    flight_t flight;
    struct Queue *next;
} queue_t;

// Statistics structure in shared memory
typedef struct Statistics {

    int total_flights;                      // Total flights created

    int total_landed;                       // Number of flights that landed
    int total_departured;                   // Number of departured flights

    int avg_waiting_time_landing;           // Average wait time to land
    int avg_waiting_time_departure;         // Average wait time to departure

    int avg_holding_maneuvers_landing;      // Average Number of holding maneuvers for landing flights
    int avg_holding_maneuvers_emergency;    // Average Number of holding maneuver for emergency flights

    int detour_flights;                     // Number of detoured flights
    int rejected_flights;                   // Number of rejected flights

    int aux_priority_flights;
} stats_t;

// Structure shared in memory
typedef struct Shared {
    pthread_cond_t listener;
    pthread_cond_t time_refresher;
    stats_t stats;
    int active_arrivals, active_departures;
    int time;
    int flight_ids[];
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

// Message structure
typedef struct FlightMessage {
    long msg_type;
    long answer_msg_type;
    int eta;
    int fuel;
    int takeoff;
    int slot;
} msg_t;


#endif // STRUCTS_H