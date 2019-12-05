
#ifndef STRUCTS_H
#define STRUCTS_H

#define DEPARTURE_FLIGHT 0
#define ARRIVAL_FLIGHT 1

#define FLIGHT_THREAD_REQUEST 2
#define FLIGHT_PRIORITY_REQUEST 1

#define HOLDING 12
#define DETOUR 13

#define NOT_APLICABLE -1

#define BUF_SIZE 1024

// Flight Structure
typedef struct ArrivalFlight {
    char name[BUF_SIZE];
    int flight_id;
    int init;
    int eta;
    int fuel;
} arrival_t;

typedef struct DepartureFlight {
    char name[BUF_SIZE];
    int flight_id;
    int init;
    int takeoff;
} departure_t;

typedef union Flight {
    arrival_t *a_flight;
    departure_t *d_flight;
} flight_t;

typedef struct Queue {
    int type;
    flight_t flight;
    struct Queue *next;
} queue_t;

typedef struct Statistics {
    int total_flights;                      //total voos criados

    int total_landed;                       //total de voos que aterraram
    int total_departured;                   //total de voos que descolaram

    //tempo médio de espera para aterrar (para além do eta)
    int avg_waiting_time_landing;           //soma de todos os tempos de espera arrivals: vai ser dividido pelo total_landed para ter a média
    //tempo médio de espera para descolar (para além do takeoff(?))
    int avg_waiting_time_departure;         //soma de todos os tempos de espera de departures: vai ser dividido pelo total_departured para ter a média
   
    int avg_holding_maneuvers_landing;      //Número médio de manobras de HOLDING para voos de aterragem
    int avg_holding_maneuvers_emergency;    //Número médio de manobras de HOLDING para voos urgentes

    int detour_flights;                     //Número de voos redirecionados
    int rejected_flights;                   //Número de voos Rejeitados pela Torre de Controlo
} stats_t;

// Structure shared in memory
typedef struct Shared {
    pthread_cond_t listener;
    pthread_cond_t time_refresher;
    stats_t stats;
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


typedef struct FlightMessage {
    long msg_type;
    long answer_msg_type;
    int eta;
    int fuel;
    int takeoff;
    int slot;
} msg_t;


#endif // STRUCTS_H