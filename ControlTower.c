/*
 *      ControlTower.c
 *
 *      Copyright 2019 Miguel Rabuge Nº 2018293728
 *      Copyright 2019 Pedro Rodrigues Nº 2018283166
 */

// C standard library includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <signal.h>
#include <unistd.h>

// Other includes
#include "SimulationUtils.h"
#include "ControlTower.h"
#include "logging.h"

// Global Variables
pthread_cond_t decrementer = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutextest = PTHREAD_MUTEX_INITIALIZER;
pthread_t talker, updater, dispatcher;
queue_t *fly_departures_queue, *land_arrivals_queue, *emergency_arrivals_queue;

void tower_manager() {
    sem_post(tower_mutex);

    // signal Handling routines
    signal(SIGUSR1, stats_show);
    signal(SIGUSR2, cleanup);

    // Create the queues to hold the flights in the control tower.
    fly_departures_queue = create_queue(DEPARTURE_FLIGHT);
    land_arrivals_queue = create_queue(ARRIVAL_FLIGHT);
    emergency_arrivals_queue = create_queue(ARRIVAL_FLIGHT);

    //  Create Control Tower Message queue communicator thread
    if (pthread_create(&talker, NULL, msq_comunicator, NULL)) {
        log_error(NULL, "msq_comunicator thread creation failed", ON);
        exit(0);
    }

    //  Create Control Tower flight updater thread
    if (pthread_create(&updater, NULL, flights_updater, NULL)) {
        log_error(NULL, "flights_updater thread creation failed", ON);
        exit(0);
    }

    //  Create Control Tower flight dispatcher thread
    if (pthread_create(&dispatcher, NULL, dispatcher_func, NULL)) {
        log_error(NULL, "dispatcher_func thread creation failed", ON);
        exit(0);
    }

    // Wait for SIGUSR1 for statistics display
    while (1) {
        pause();
        signal(SIGUSR1, stats_show);
    }
}

// Talker thread that sends receives messages and indicates the shared memory slot to the flights
void *msq_comunicator(void *nothing) {
    while (1) {
        int i;
        msg_t message;
        departure_t *departure;
        arrival_t *arrival;

        // Receive slot information from the flight thread
        if (msgrcv(msqid, &message, sizeof(message), -FLIGHT_THREAD_REQUEST, 0) < 0) {
            log_error(NULL, "Control Tower: Failed to receive message from the flight!", ON);
        }

        // Find empty slot in shared memory
        sem_wait(shm_mutex);
        for (i = 0; (i < num_flights) && (shm_struct->flight_ids[i] != STATE_FREE); i++);
        sem_post(shm_mutex);

        // Handle messages and select the appropriate one for the given flight
        if (i < num_flights) {
            message.slot = i;

            if (message.fuel == NOT_APLICABLE) { //  DEPARTURE
                sem_wait(shm_mutex);

                if (shm_struct->active_departures < configs.max_departures) {
                    sem_post(shm_mutex);

                    // New departure flight creation for tracking
                    departure = (departure_t *) malloc(sizeof(departure_t));
                    strcpy(departure->name, "NOT_APLICABLE");

                    // init becomes the takeoff so we can reuse linked list functions
                    departure->init = message.takeoff;
                    departure->flight_id = message.slot;

                    // Adding flight to the queues
                    add_departure(fly_departures_queue, departure);
                    shm_struct->flight_ids[i] = STATE_OCCUPIED;
                } else {
                    sem_post(shm_mutex);
                    message.slot = NOT_APLICABLE;
                }

            } else {  // ARRIVAL
                sem_wait(shm_mutex);

                if (shm_struct->active_arrivals < configs.max_arrivals) {
                    sem_post(shm_mutex);

                    // New arrival flight creation for tracking
                    arrival = (arrival_t *) malloc(sizeof(arrival_t));
                    strcpy(arrival->name, "NOT_APLICABLE");

                    // init becomes the eta so we can reuse linked list functions
                    arrival->init = message.eta;
                    arrival->eta = message.takeoff;
                    arrival->fuel = message.fuel;
                    arrival->flight_id = message.slot;

                    // Adding flight to the queues
                    if (message.msg_type == FLIGHT_PRIORITY_REQUEST) {
                        add_arrival_TC(emergency_arrivals_queue, arrival);
                    } else {
                        add_arrival_TC(land_arrivals_queue, arrival);
                    }
                    shm_struct->flight_ids[i] = STATE_OCCUPIED;
                } else {
                    sem_post(shm_mutex);
                    message.slot = NOT_APLICABLE;
                }
            }

        } else { // FULL
            message.slot = NOT_APLICABLE;
        }
        message.msg_type = message.answer_msg_type;

        // Sending message to the flight
        if (msgsnd(msqid, &message, sizeof(message) - sizeof(long), IPC_NOWAIT)) {
            log_error(NULL, "Failed to send message from the Control Tower", ON);
        }
    }
}

void stats_show(int signo) {
    signal(SIGUSR1, SIG_IGN);
    sem_wait(shm_mutex);
    log_info(NULL, "SIGUSR1 called --> Statistics!", ON);

    printf("\n\t<----------------------------------------------------------------------->\n");
    printf("\t\t# Total flights created: %d\n", shm_struct->stats.total_flights);
    printf("\t\t   -> LANDED FLIGHTS: %d\n", shm_struct->stats.total_landed);
    printf("\t\t   -> DEPARTURED FLIHTS: %d\n", shm_struct->stats.total_departured);

    printf("\t\t# Average wait time to land: %.2f\n",
           shm_struct->stats.avg_waiting_time_landing / (double) shm_struct->stats.total_landed);

    printf("\t\t# Average wait time to departure: %.2f\n",
           shm_struct->stats.avg_waiting_time_departure / (double) shm_struct->stats.total_departured);

    printf("\t\t# Average number of holding maneuvers for landing flights: %.2f\n",
           shm_struct->stats.avg_holding_maneuvers_landing / (double) shm_struct->stats.total_landed);

    printf("\t\t# Average number of holding maneuver for emergency flights: %.2f\n",
           shm_struct->stats.avg_holding_maneuvers_emergency / (double) shm_struct->stats.aux_priority_flights);

    printf("\t\t# Number of detoured flights: %d\n", shm_struct->stats.detour_flights);
    printf("\t\t# Number of rejected flights: %d\n", shm_struct->stats.rejected_flights);
    printf("\t<------------------------------------------------------------------------->\n\n");
    sem_post(shm_mutex);
}

void *flights_updater(void *nothing) {
    queue_t *current;
    int flight_counter, time;

    while (1) {
        pthread_mutex_lock(&mutextest);
        pthread_cond_wait(&(shm_struct->time_refresher), &mutextest);

        current = land_arrivals_queue->next;
        flight_counter = 0;
        time = get_time();

       log_info(NULL,"Updating Fuel...",ON);

        while (current) {
            --(current->flight.a_flight->fuel);
            printf("Fuel : %d   %d\n", current->flight.a_flight->fuel, current->flight.a_flight->init);
            if ((current->flight.a_flight->fuel) <=
                (4 + (current->flight.a_flight->init - current->flight.a_flight->eta) + configs.landing_time)) {
                sem_wait(shm_mutex);
                shm_struct->flight_ids[current->flight.a_flight->flight_id] = EMERGENCY;
                pthread_mutex_lock(&mutex_arrivals);
                remove_flight_TC(land_arrivals_queue, current->flight.a_flight->flight_id, NULL);
                pthread_mutex_unlock(&mutex_arrivals);
                pthread_cond_broadcast(&(shm_struct->listener));
                sem_post(shm_mutex);
            }
            current = current->next;
        }
        current = emergency_arrivals_queue->next;

        while (current) {
            ++flight_counter;
            --(current->flight.a_flight->fuel);
            printf("Fuel esp: %d    %d\n", (current->flight.a_flight->fuel), current->flight.a_flight->init);
            if ((current->flight.a_flight->fuel) == 0) {
                log_info(NULL, "Zero Fuel!!", ON);
                sem_wait(shm_mutex);

                shm_struct->flight_ids[current->flight.a_flight->flight_id] = DETOUR;
                pthread_mutex_lock(&mutex_arrivals);
                remove_flight_TC(emergency_arrivals_queue, current->flight.a_flight->flight_id, NULL);
                pthread_mutex_unlock(&mutex_arrivals);
                pthread_cond_broadcast(&(shm_struct->listener));

                sem_post(shm_mutex);
            }
            if ((flight_counter > 5) && (current->flight.a_flight->init == time)) {
                sem_wait(shm_mutex);

                shm_struct->flight_ids[current->flight.a_flight->flight_id] = HOLDING;
                pthread_mutex_lock(&mutex_arrivals);
                remove_flight_TC(emergency_arrivals_queue, current->flight.a_flight->flight_id, NULL);
                pthread_mutex_unlock(&mutex_arrivals);
                pthread_cond_broadcast(&(shm_struct->listener));

                sem_post(shm_mutex);
            }
            current = current->next;
        }
        pthread_cond_broadcast(&decrementer);
        pthread_mutex_unlock(&mutextest);
    }

}

void *dispatcher_func(void *nothing) {
    int i, time, any_to_fly_flag;
    while (1) {

        any_to_fly_flag = 0;
        pthread_mutex_lock(&mutextest);
        pthread_cond_wait(&decrementer, &mutextest);
        time = get_time();

        // Wait for a free runway to stagger more flights!
        log_info(NULL, "Checking runways...", ON);

        // If there is a priority arrival flight then land that flight
        if (emergency_arrivals_queue->next && emergency_arrivals_queue->next->flight.a_flight->init <= time) {
            sem_wait(shm_mutex);
            shm_struct->flight_ids[emergency_arrivals_queue->next->flight.a_flight->flight_id] = FLY_LAND;
            sem_post(shm_mutex);

            pthread_mutex_lock(&mutex_arrivals);
            remove_flight_TC(emergency_arrivals_queue, emergency_arrivals_queue->next->flight.a_flight->flight_id,NULL);
            pthread_mutex_unlock(&mutex_arrivals);

            // Check for one more priority flight!
            if (emergency_arrivals_queue->next && emergency_arrivals_queue->next->flight.a_flight->init <= time) {
                sem_wait(shm_mutex);
                shm_struct->flight_ids[emergency_arrivals_queue->next->flight.a_flight->flight_id] = FLY_LAND;
                sem_post(shm_mutex);

                pthread_mutex_lock(&mutex_arrivals);
                remove_flight_TC(emergency_arrivals_queue, emergency_arrivals_queue->next->flight.a_flight->flight_id,NULL);
                pthread_mutex_unlock(&mutex_arrivals);


                // If there is no second priority arrival then search for one in the arrival queue
            } else if (land_arrivals_queue->next && land_arrivals_queue->next->flight.a_flight->init <= time) {
                sem_wait(shm_mutex);
                shm_struct->flight_ids[land_arrivals_queue->next->flight.a_flight->flight_id] = FLY_LAND;
                sem_post(shm_mutex);
                pthread_mutex_lock(&mutex_arrivals);
                remove_flight_TC(land_arrivals_queue, land_arrivals_queue->next->flight.a_flight->flight_id, NULL);
                pthread_mutex_unlock(&mutex_arrivals);
            }
            pthread_cond_broadcast(&(shm_struct->listener));
            any_to_fly_flag = 1;


            // Get one or two flights from the departure queue instead
        } else if (fly_departures_queue->next != NULL && fly_departures_queue->next->flight.d_flight->init <= time) {
            for (i = 0; i < 2 && fly_departures_queue->next && (fly_departures_queue->next->flight.d_flight->init <= time); i++) {

                sem_wait(shm_mutex);
                shm_struct->flight_ids[fly_departures_queue->next->flight.d_flight->flight_id] = FLY_LAND;
                sem_post(shm_mutex);

                pthread_mutex_lock(&mutex_departures);
                remove_flight_TC(fly_departures_queue, fly_departures_queue->next->flight.d_flight->flight_id, NULL);
                pthread_mutex_unlock(&mutex_departures);
            }
            pthread_cond_broadcast(&(shm_struct->listener));
            any_to_fly_flag = 1;


            // If there is no emergency flight to takeoff then go to the arrivals queue and select one or two flights
        } else if (land_arrivals_queue->next && land_arrivals_queue->next->flight.a_flight->init <= time) {
            for (i = 0; i < 2 && land_arrivals_queue->next && (land_arrivals_queue->next->flight.a_flight->init <= time); i++) {

                sem_wait(shm_mutex);
                shm_struct->flight_ids[land_arrivals_queue->next->flight.a_flight->flight_id] = FLY_LAND;
                sem_post(shm_mutex);

                pthread_mutex_lock(&mutex_arrivals);
                remove_flight_TC(land_arrivals_queue, land_arrivals_queue->next->flight.a_flight->flight_id, NULL);
                pthread_mutex_unlock(&mutex_arrivals);
            }

            pthread_cond_broadcast(&(shm_struct->listener));
            any_to_fly_flag = 1;
        }
        pthread_mutex_unlock(&mutextest);

        if (any_to_fly_flag) {
            sem_wait(runway_mutex);
        }

        sem_wait(runway_mutex);
        sem_post(runway_mutex);
    }
}

void cleanup(int signo) {
    log_info(NULL, "Control Tower Process cleanup...", ON);

    pthread_cancel(updater);
    pthread_join(updater, NULL);
    pthread_cancel(dispatcher);

    pthread_cancel(talker);
    pthread_join(talker, NULL);

    delete_queue(fly_departures_queue);
    delete_queue(land_arrivals_queue);
    delete_queue(emergency_arrivals_queue);
    pthread_mutex_destroy(&mutextest);
    pthread_cond_destroy(&decrementer);

    log_info(NULL, "DONE! (Control Tower Process cleanup!)", ON);

    exit(0);
}
