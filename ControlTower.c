#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#include "ControlTower.h"

void tower_manager(){
    sem_post(tower_mutex);
    while(1){
        
    }
}