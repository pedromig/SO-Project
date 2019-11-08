#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>

#define PIPE_NAME "input_pipe"
#define BUF_SIZE 1024

//struct in shared memory
typedef struct shared{
    //estatisticas
	int tempo;
}shared_t;

//flight struct
typedef struct flight{
    int type;
    char name[BUF_SIZE];
    int init;
    int takeoff;
    int eta;
    int fuel;
}flight_t;

//flight linkedlist queue
typedef struct queue{
    flight_t *flight;
    struct queue * next;
}queue_t;

//id's
int shmid, msqid;
shared_t * shm_struct;
pthread_t timer_thread;
/*int read_config(char* file_dir);
int tower_manager();*/

//timer funcs
void* timer(void * time_units){
    int time = *((int *) time_units);
    time = time*1000;
    while(1){
        shm_struct->tempo++;
        usleep(time);
        printf("time: %d\n",shm_struct->tempo);
    }
}

int get_time(){
    return shm_struct->tempo;
}
//parsing/queue pre-setup

int isnumber(char* string);
flight_t * check_arrival(char* buffer,int current_time);
flight_t * check_departure(char* buffer,int current_time);

//queue (linked list) funcs
void add_queue(queue_t* head, flight_t* flight){
    queue_t *last = head;
    queue_t *current = head->next;
    queue_t *new;
    while(current && (current->flight->init < flight->init)){
        current = current->next;
        last = last -> next;
    }
    new = malloc(sizeof(queue_t));
    new->flight = flight;
    new->next = current;
    last->next = new;
}

void print_queue(queue_t * head){
    queue_t * current = head->next;
    char *type[2];
    type[0] = "DEPARTURE";
    type[1] = "ARRIVAL";
    printf("Booked Flights:\n");
    while(current){
        printf("[%s init:%d]\n",type[current->flight->type],current->flight->init);
        current = current->next;
    }
}

int main(){
    int time_control = 0;
    int tempoo;
    int words,fd, n_read;
    char buffer[BUF_SIZE], save[BUF_SIZE], *token;
    pid_t control_tower;
    flight_t *booker;
    queue_t * queue;
    // read configurations
    tempoo = 2000;//test value
    /*if(read_config("diretorio")){
        printf("Erro: Configurações!\n");
        exit(0);
    }*/
    //shared memory
    if((shmid = shmget(IPC_PRIVATE, sizeof(shared_t), IPC_CREAT | 0777)) < 0){
        printf("Erro: shmget!\n");
        exit(0);
    }
    shm_struct = (shared_t*) shmat(shmid,NULL,0);
    if (shm_struct == (shared_t*) -1) {
        printf("Erro: shmat!\n");
        exit(0);
    }
    //start program timer 
    shm_struct->tempo = 0;
    if(pthread_create(&timer_thread,NULL,timer,(void*) &tempoo)){
        printf("Timer initialization failed!\n");
        exit(0);
    }
 
    //message queue
    if((msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0777)) == -1){
        printf("Erro: msgget!\n");
        exit(0);
    }
    
    //named pipe
    unlink(PIPE_NAME);
    if (mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0777) < 0){
        printf("Erro: Pipe!");
        exit(0);
    }
	//queue initialization
    queue = malloc(sizeof(queue_t));
    queue->flight = NULL;
    queue->next = NULL;
    //need thread for check if init == current_time
    //control_tower process creation
    if((control_tower = fork()) == 0){
        //tower_manager();
	    printf("control tower activated!\n");
        exit(0);
    }
    //simulator
    while(1){
        //wait for something in the input_pipe
        if ((fd = open(PIPE_NAME, O_RDONLY)) < 0) {
            printf("Cannot open pipe for reading!\n ");
            exit(0);
        }
        //when "open()" unblocks, read input_pipe into the buffer
        n_read = read(fd,buffer,BUF_SIZE);
        time_control = get_time();
        buffer[n_read-1] = '\0';//terminate with \0
        strcpy(save,buffer);//saved copy of the buffer
        //print message received from pipe
        printf("RECEIVED => %s\n",buffer);
        //process the information
        token = strtok(buffer," ");
        words = 0;
        while(token){
            token = strtok(NULL," ");
            words++;
        }
        if( words == 6 ){//DEPARTURE
            if(booker = check_departure(save,time_control)){
                add_queue(queue,booker);
                printf("Departure queued!\n");
                printf("    FLIGHT: %s\n    init: %d\n    takeoff: %d\n",booker->name,booker->init,booker->takeoff);
                
                print_queue(queue);
            } else {
                printf("Failed to book Flight!\n");
            }
        } else if ( words == 8 ){//ARRIVAL
            if(booker = check_arrival(save,time_control)){
                add_queue(queue,booker);
                printf("Arrival queued!\n");
                printf("    FLIGHT: %s\n    init: %d\n    eta: %d\n    fuel: %d\n",booker->name,booker->init,booker->eta,booker->fuel);
                print_queue(queue);
            } else {
                printf("Failed to book Landing!\n");
            }
        } else {
            printf("Bad request!\n");
        }
        //close pipe file descriptor
        booker = NULL;
        close(fd);
    }
    
}
flight_t * check_arrival(char* buffer, int current_time){
    char *aux;
    char *keyword;
    char *flight_code;
    int init, takeoff, eta, fuel;
    flight_t * flight;
    //ARRIVAL
    keyword = strtok(buffer," ");
    if(strcmp(keyword,"ARRIVAL")){
        return NULL;
    }
    //flight_code
    flight_code = strtok(NULL," ");
    //init:
    aux = strtok(NULL," ");
    if (strcmp(aux,"init:")){
        return NULL;
    }
    // init value
    aux = strtok(NULL," ");
    if(!isnumber(aux)){
        return NULL;
    } else {
        init = atoi(aux);
        if(init< current_time){
            return NULL;
        }
    }
    //eta:
    aux = strtok(NULL," ");
    if (strcmp(aux,"eta:")){
        return NULL;
    }
    //eta value
    aux = strtok(NULL," ");
    if(!isnumber(aux)){
        return NULL;
    } else {
        eta = atoi(aux);
    }
    //fuel:
    aux = strtok(NULL," ");
    if (strcmp(aux,"fuel:")){
        return NULL;
    }
    //fuel value
    aux = strtok(NULL," ");
    if(!isnumber(aux)){
        return 0;
    } else {
        fuel = atoi(aux);
        if(fuel < eta){
            return NULL;
        }
    }
    flight = malloc(sizeof(flight_t));
    flight->type = 1;
    strcpy(flight->name,flight_code);
    flight->init = init;
    flight->eta = eta;
    flight->fuel = fuel;
    flight->takeoff = -1;
    return flight;
}
flight_t *check_departure(char* buffer,int current_time){
    char *aux;
    char *keyword;
    char *flight_code;
    int init, takeoff;
    flight_t *flight;
    //DEPARTURE
    keyword = strtok(buffer," ");
    if(strcmp(keyword,"DEPARTURE")){
        return NULL;
    }
    //flight_code
    flight_code = strtok(NULL," ");
    //init:
    aux = strtok(NULL," ");
    if (strcmp(aux,"init:")){
        return NULL;
    }
    // init value
    aux = strtok(NULL," ");
    if(!isnumber(aux)){
        return NULL;
    } else {
        init = atoi(aux);
        if (init < current_time){
            return NULL;
        }
    }
    //takeoff:
    aux = strtok(NULL," ");
    if (strcmp(aux,"takeoff:")){
        return NULL;
    }
    //takeoff value
    aux = strtok(NULL," ");
    if(!isnumber(aux)){
        return NULL;
    } else {
        takeoff = atoi(aux);
        if ((takeoff < current_time) || (takeoff <= init)){
            return NULL;
        }
    }
    flight = malloc(sizeof(flight_t));
    
    flight->type = 0;
    strcpy(flight->name, flight_code);
    flight->init = init;
    flight->takeoff = takeoff;
    flight->eta = -1;
    flight->fuel = -1;

    return flight;
}
int isnumber(char* string){
    int len = strlen(string);
    for (int i = 0; i < len; i++){
        if(!isdigit(string[i])){
            return 0;
        }
    }
    return 1;
}