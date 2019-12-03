#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include "SimulationUtils.h"
#include "ControlTower.h"
#include "logging.h"


pthread_t talker;
queue_t *fly_departures_queue, *land_arrivals_queue;

void tower_manager() {
    sem_post(tower_mutex);
    signal(SIGINT,SIG_IGN);
    signal(SIGUSR1,stats_show);
    signal(SIGUSR2,cleanup);

    fly_departures_queue = create_queue(DEPARTURE_FLIGHT);
    land_arrivals_queue = create_queue(ARRIVAL_FLIGHT);

    if(pthread_create(&talker,NULL,msq_comunicator,NULL)){
        printf("Erro.\n");
        exit(0);
    }

    while(1){
        pause();
        signal(SIGUSR1,stats_show);
    }
    exit(0);
}
/**
 * Lê os pedidos, mete nas queues e diz às threads que slot na shm devem escutar
 */
void* msq_comunicator(void* nothing){
    while (1) {
        int i;
        msg_t message;
        departure_t *departure;
        arrival_t *arrival;
        if (msgrcv(msqid, &message, sizeof(message), FLIGHT_THREAD_REQUEST, 0) < 0) {
            log_error(NULL, "Control Tower: Failed to receive message from the flight!", ON);
        }
        sem_wait(shm_mutex);
        for (i = 0; (i < num_flights) && (shm_struct->flight_ids[i] != STATE_FREE); i++); // iterar até encontrar um slot a zero
        sem_post(shm_mutex);
        if (i < num_flights){ //isto pq o "i" pode falhar pela primeira condição: com (i == num_flights) e (shm_struct->flight_ids[int]) não é possivel...
            message.slot = i;   
            if( message.fuel == NOT_APLICABLE ){ // Significa que é um departure
                departure = (departure_t*) malloc(sizeof(departure_t));
                strcpy(departure->name,"NOT_APLICABLE");
                departure->init = message.takeoff; // as funcs das queues já estão programadas para ordenar pelo init, então faz se este trick, e assim ordena se pelo takeoff
                departure->takeoff = NOT_APLICABLE;
                add_departure(fly_departures_queue,departure);
            } else if( message.takeoff == NOT_APLICABLE  ){ // Significa que é um arrival TODO: ambiguo probably (porque ou é uma ou é outra, esta proteção a mais pode não fazer sentido)
                arrival = (arrival_t*) malloc(sizeof(arrival_t));
                strcpy(arrival->name,"NOT_APLICABLE");
                arrival->init = message.eta; // mesmo trick (ordenar pelo ETA como diz no enunciado)
                arrival->eta = NOT_APLICABLE;
                arrival->fuel = message.fuel;
                add_arrival(land_arrivals_queue,arrival);
                arrival = NULL;
            }
            message.msg_type = message.answer_msg_type;
            shm_struct->flight_ids[i] = STATE_OCCUPIED;
        } else {
            //TODO: ERRO de todos os slots estarem ocupados e portanto o voo deve ser descartado (?)
        }
        
        if (msgsnd(msqid, &message, sizeof(message) - sizeof(long), IPC_NOWAIT)) {
            log_error(NULL, "Failed to send message from the Control Tower", ON);
        }
    }
}

void stats_show(int signo) {
    signal(SIGUSR1,SIG_IGN);
    printf("Show Stats\n");
}

void cleanup(int signo) {
    printf("cleanup\n");
    pthread_cancel(talker);
    pthread_join(talker,NULL);
    delete_queue(fly_departures_queue);
    delete_queue(land_arrivals_queue);

    exit(0);
}
