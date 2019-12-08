/*
 *      SimulationManager.c
 *
 *      Copyright 2019 Miguel Rabuge
 *      Copyright 2019 Pedro Rodrigues
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <signal.h>
#include <unistd.h>

#include "SimulationUtils.h"
#include "ControlTower.h"
#include "logging.h"

pthread_cond_t decrementer = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutextest = PTHREAD_MUTEX_INITIALIZER; //TODO: change name
pthread_t talker, updater, dispatcher;
queue_t *fly_departures_queue, *land_arrivals_queue, *emergency_arrivals_queue;

void tower_manager() {
    sem_post(tower_mutex);

    signal(SIGUSR1,stats_show);
    signal(SIGUSR2,cleanup);

    fly_departures_queue = create_queue(DEPARTURE_FLIGHT);
    land_arrivals_queue = create_queue(ARRIVAL_FLIGHT);
    emergency_arrivals_queue = create_queue(ARRIVAL_FLIGHT);
    if(pthread_create(&talker,NULL,msq_comunicator,NULL)){
        printf("Erro\n");
        exit(0);
    }
    if(pthread_create(&updater,NULL,flights_updater,NULL)){
        printf("Erro\n");
        exit(0);
    }
    if(pthread_create(&dispatcher,NULL,dispatcher_func,NULL)){
        printf("Erro\n");
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
        if (msgrcv(msqid, &message, sizeof(message), -FLIGHT_THREAD_REQUEST, 0) < 0) {
            log_error(NULL, "Control Tower: Failed to receive message from the flight!", ON);
        }
        sem_wait(shm_mutex);
        for (i = 0; (i < num_flights) && (shm_struct->flight_ids[i] != STATE_FREE); i++); // iterar até encontrar um slot a zero
        sem_post(shm_mutex);
        if (i < num_flights){ //isto pq o "i" pode falhar pela primeira condição: com (i == num_flights) e (shm_struct->flight_ids[int]) não é possivel...
            message.slot = i;
            if( message.fuel == NOT_APLICABLE ){ // Significa que é um departure
                sem_wait(shm_mutex);
                if(shm_struct->active_departures < configs.max_departures){
                    sem_post(shm_mutex);
                    departure = (departure_t*) malloc(sizeof(departure_t));
                    strcpy(departure->name,"NOT_APLICABLE");
                    departure->init = message.takeoff; // o init passa a ser o momento em que os voos querem descolar
                    departure->flight_id = message.slot;
                    add_departure(fly_departures_queue,departure);
                    shm_struct->flight_ids[i] = STATE_OCCUPIED;
                } else {
                    sem_post(shm_mutex);
                    message.slot = NOT_APLICABLE;
                }
            } else { // caso contrário significa que é um arrival 
                sem_wait(shm_mutex);
                if (shm_struct->active_arrivals < configs.max_arrivals){
                    sem_post(shm_mutex);
                    arrival = (arrival_t*) malloc(sizeof(arrival_t));
                    strcpy(arrival->name,"NOT_APLICABLE");
                    arrival->init = message.eta; // o init passa a ser o momento em que os voos querem descolar
                    arrival->eta = message.takeoff; //eta == original_Eta
                    arrival->fuel = message.fuel;
                    arrival->flight_id = message.slot;
                    if (message.msg_type == FLIGHT_PRIORITY_REQUEST){
                        add_arrival_TC(emergency_arrivals_queue,arrival);
                    } else {
                        add_arrival_TC(land_arrivals_queue,arrival);
                    }
                    shm_struct->flight_ids[i] = STATE_OCCUPIED;
                } else {
                    sem_post(shm_mutex);
                    message.slot = NOT_APLICABLE;
                }
            }
        } else {
            message.slot = NOT_APLICABLE;           //TUDO CHEIO
        }
        message.msg_type = message.answer_msg_type;
        if (msgsnd(msqid, &message, sizeof(message) - sizeof(long), IPC_NOWAIT)) {
            log_error(NULL, "Failed to send message from the Control Tower", ON);
        }
    }
}

void stats_show(int signo) {
    signal(SIGUSR1,SIG_IGN);
    sem_wait(shm_mutex);
    log_info(NULL,"SIGUSR1 called --> Estatísticas!",ON);
    printf("Total de voos criados: %d\n",shm_struct->stats.total_flights);
    printf("    Total de voos que aterraram: %d\n",shm_struct->stats.total_landed);
    printf("    Total de voos que descolaram: %d\n",shm_struct->stats.total_departured);
    printf("Tempo médio de espera para aterrar: %.2f\n",shm_struct->stats.avg_waiting_time_landing/(double)shm_struct->stats.total_landed);
    printf("Tempo médio de espera para descolar: %.2f\n",shm_struct->stats.avg_waiting_time_departure/(double)shm_struct->stats.total_departured);
    printf("Número médio de manobras de HOLDING para voos de aterragem: %.2f\n",shm_struct->stats.avg_holding_maneuvers_landing/(double)shm_struct->stats.total_landed);
    printf("Número médio de manobras de HOLDING para voos urgentes: %.2f\n",shm_struct->stats.avg_holding_maneuvers_emergency/(double)shm_struct->stats.aux_priority_flights);
    printf("Número de voos redirecionados: %d\n",shm_struct->stats.detour_flights);
    printf("Número de voos Rejeitados pela Torre de Controlo: %d\n",shm_struct->stats.rejected_flights);
    sem_post(shm_mutex);
}

void* flights_updater(void* nothing){
    queue_t *current;
    int flight_counter,time;
    while(1){
        pthread_mutex_lock(&mutextest);
        pthread_cond_wait(&(shm_struct->time_refresher),&mutextest);
        current = land_arrivals_queue->next;
        flight_counter = 0;
        time = get_time();
        printf("Updating Fuel!\n");
        while(current){
            --(current->flight.a_flight->fuel);
            printf("Fuel : %d   %d\n",current->flight.a_flight->fuel,current->flight.a_flight->init);
            if ((current->flight.a_flight->fuel) <= (4 + (current->flight.a_flight->init - current->flight.a_flight->eta) + configs.landing_time)){
                sem_wait(shm_mutex);
                shm_struct->flight_ids[current->flight.a_flight->flight_id] = EMERGENCY;
                pthread_mutex_lock(&mutex_arrivals);
                remove_flight_TC(land_arrivals_queue,current->flight.a_flight->flight_id,NULL);
                pthread_mutex_unlock(&mutex_arrivals);
                pthread_cond_broadcast(&(shm_struct->listener));
                sem_post(shm_mutex);
            }
            current = current->next;
        }

        current = emergency_arrivals_queue->next;

        while(current){
            ++flight_counter;
            --(current->flight.a_flight->fuel);
            printf("Fuel esp: %d    %d\n",(current->flight.a_flight->fuel),current->flight.a_flight->init);
            if((current->flight.a_flight->fuel) == 0){
                printf("É zero!\n");
                sem_wait(shm_mutex);
                shm_struct->flight_ids[current->flight.a_flight->flight_id] = DETOUR;
                pthread_mutex_lock(&mutex_arrivals);
                remove_flight_TC(emergency_arrivals_queue,current->flight.a_flight->flight_id,NULL);
                pthread_mutex_unlock(&mutex_arrivals);
                pthread_cond_broadcast(&(shm_struct->listener));
                sem_post(shm_mutex);
            }
            if ((flight_counter > 5) && (current->flight.a_flight->init == time)){
                sem_wait(shm_mutex);
                shm_struct->flight_ids[current->flight.a_flight->flight_id] = HOLDING;
                pthread_mutex_lock(&mutex_arrivals);
                remove_flight_TC(emergency_arrivals_queue,current->flight.a_flight->flight_id,NULL);
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

void* dispatcher_func(void* nothing){
    int i,time,any_to_fly_flag;
    while(1){
        any_to_fly_flag = 0;
        pthread_mutex_lock(&mutextest);
        pthread_cond_wait(&decrementer,&mutextest);
        time = get_time();
        //espera que a pista fique livre para escalonar mais voos
        printf("Checking...\n");
        if(emergency_arrivals_queue->next && emergency_arrivals_queue->next->flight.a_flight->init <= time){ // se tiver algum arrival prioritário, então aterra
            sem_wait(shm_mutex);
            shm_struct->flight_ids[emergency_arrivals_queue->next->flight.a_flight->flight_id] = FLY_LAND;
            sem_post(shm_mutex);
            pthread_mutex_lock(&mutex_arrivals);
            remove_flight_TC(emergency_arrivals_queue,emergency_arrivals_queue->next->flight.a_flight->flight_id,NULL);
            pthread_mutex_unlock(&mutex_arrivals);
            if(emergency_arrivals_queue->next && emergency_arrivals_queue->next->flight.a_flight->init <= time){// se tiver outro arrival prioritário, também aterra
                sem_wait(shm_mutex);
                shm_struct->flight_ids[emergency_arrivals_queue->next->flight.a_flight->flight_id] = FLY_LAND;
                sem_post(shm_mutex);
                pthread_mutex_lock(&mutex_arrivals);
                remove_flight_TC(emergency_arrivals_queue,emergency_arrivals_queue->next->flight.a_flight->flight_id,NULL);
                pthread_mutex_unlock(&mutex_arrivals);
            } else if(land_arrivals_queue->next && land_arrivals_queue->next->flight.a_flight->init <= time){// se não tiver um segundo arrival prioritário, vê se tem algum nos arrivals normais
                sem_wait(shm_mutex);
                shm_struct->flight_ids[land_arrivals_queue->next->flight.a_flight->flight_id] = FLY_LAND;
                sem_post(shm_mutex);
                pthread_mutex_lock(&mutex_arrivals);
                remove_flight_TC(land_arrivals_queue,land_arrivals_queue->next->flight.a_flight->flight_id,NULL);
                pthread_mutex_unlock(&mutex_arrivals);
            }
            pthread_cond_broadcast(&(shm_struct->listener));
            any_to_fly_flag = 1;
        } else if (fly_departures_queue->next != NULL && fly_departures_queue->next->flight.d_flight->init <= time){ // caso contrário, vai ao departures e descolam 1 ou 2 voos
            for (i = 0; i < 2 && fly_departures_queue->next && (fly_departures_queue->next->flight.d_flight->init <= time); i++){
                sem_wait(shm_mutex);
                shm_struct->flight_ids[fly_departures_queue->next->flight.d_flight->flight_id] = FLY_LAND;
                sem_post(shm_mutex);
                pthread_mutex_lock(&mutex_departures);
                remove_flight_TC(fly_departures_queue,fly_departures_queue->next->flight.d_flight->flight_id,NULL);
                pthread_mutex_unlock(&mutex_departures);
            }
            pthread_cond_broadcast(&(shm_struct->listener));
            any_to_fly_flag = 1;
        } else if(land_arrivals_queue->next &&land_arrivals_queue->next->flight.a_flight->init <= time) {//caso não houver nenhum voo prioritário nem nenhum para descolar, vai ao arrivals, e aterram 1 ou 2 voos
            for (i = 0; i < 2 && land_arrivals_queue->next && (land_arrivals_queue->next->flight.a_flight->init <= time); i++){
                sem_wait(shm_mutex);
                shm_struct->flight_ids[land_arrivals_queue->next->flight.a_flight->flight_id] = FLY_LAND;
                sem_post(shm_mutex);
                pthread_mutex_lock(&mutex_arrivals);
                remove_flight_TC(land_arrivals_queue,land_arrivals_queue->next->flight.a_flight->flight_id,NULL);
                pthread_mutex_unlock(&mutex_arrivals);
            }
            pthread_cond_broadcast(&(shm_struct->listener));
            any_to_fly_flag = 1;
        }
        pthread_mutex_unlock(&mutextest);
        if(any_to_fly_flag){
            sem_wait(runway_mutex);
        }
        sem_wait(runway_mutex);
        sem_post(runway_mutex);
    }
}

void cleanup(int signo) {
    printf("cleanup\n");

    pthread_cancel(updater);
    pthread_join(updater,NULL);
    pthread_cancel(dispatcher);

    //pthread_join(dispatcher,NULL);

    pthread_cancel(talker);
    pthread_join(talker,NULL);

    delete_queue(fly_departures_queue);
    delete_queue(land_arrivals_queue);
    delete_queue(emergency_arrivals_queue);
    pthread_mutex_destroy(&mutextest);
    pthread_cond_destroy(&decrementer);
    printf("cleanup end\n");
    exit(0);
}
