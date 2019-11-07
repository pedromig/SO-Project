
#ifndef STRUCTS_H
#define STRUCTS_H

#define ARRIVAL_FLIGHT 0
#define DEPARTURE_FLIGHT 1

typedef struct Flight{
    char *name;
    int init_time;
    int takeoff_time;
    int eta;
    int init_fuel;
    int type;
}flight_t;
 
typedef struct Shared{
    flight_t flight;
    int time;
}shared_t;

typedef struct Configurations{
    int time_units;

    int takeoff_time;
    int takeoff_gap;

    int landing_time;
    int landing_gap;

    int holding_min;
    int holding_max;

    int max_departures;
    int max_arrivals;

}config_t;

#endif // STRUCTS_H