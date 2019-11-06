
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


#endif // STRUCTS_H