#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "kernel.h"

void *prozesu_sortzailea(void *this) {
    //int *this_t = (int*)this;
    //thread this_thread = cpus.cct[this_t[0]][this_t[1]][this_t[2]];
    int level;
    pthread_mutex_lock(&mutex_ps);
    while(1) {
        pthread_cond_wait(&cond_ps, &mutex_ps);
        if (scheduler_politic == SCHEDULER_POLITIC_RORO)
            level = 0;
        else
            level = (rand()%PRIORITY_LEVELS);
        
        pthread_mutex_lock(&mutex_proccess_queue);
        if (last_p[level] != first_p[level] || proccess_queue[level][first_p[level]].state == STATE_UNDEFINED) {
            proccess_queue[level][last_p[level]].id = next_p_id;
            proccess_queue[level][last_p[level]].execution_time_needed = (rand()%(int)pow(2, PRIORITY_LEVELS-1))+1;
            proccess_queue[level][last_p[level]].time_executed = 0;
            if (scheduler_politic == SCHEDULER_POLITIC_RORO)
                proccess_queue[level][last_p[level]].quantum = (rand()%proccess_queue[level][last_p[level]].execution_time_needed)+1;
            else
                proccess_queue[level][last_p[level]].quantum = pow(2, level);
            proccess_queue[level][last_p[level]].level = level;
            proccess_queue[level][last_p[level]].state = STATE_READY;
            next_p_id++;
            printf("%s%lu prozesua sortuta.\n", KCYN, proccess_queue[level][last_p[level]].id);
            last_p[level] = (last_p[level]+1)%PROC_KOP_MAX;
        }
        else
            printf("%sProzesu kopuru maximoa prozesuaren mailan. Ezin da prozesurik sortu.\n", KCYN);
        pthread_mutex_unlock(&mutex_proccess_queue);
    }
}