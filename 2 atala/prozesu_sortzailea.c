#include "kernel.h"

void *prozesu_sortzailea() {
    int level;
    pthread_mutex_lock(&mutex_ps);
    while(1) {
        pthread_cond_wait(&cond_ps, &mutex_ps);
        if (finish)
            break;
        if (scheduler_politic == SCHEDULER_POLITIC_RORO)
            level = 0;
        else if (scheduler_politic == SCHEDULER_POLITIC_LDDQ)
            level = (rand()%PRIORITY_LEVELS);
        pthread_mutex_lock(&mutex_proccess_queue);
        if (last_p[level] != first_p[level] || proccess_queue[level][first_p[level]].state == STATE_UNDEFINED) {
            PCB proccess;
            proccess.id = next_p_id;
            proccess.execution_time_needed = (rand()%(int)pow(2, PRIORITY_LEVELS-1))+1;
            proccess.time_executed = 0;
            if (scheduler_politic == SCHEDULER_POLITIC_RORO)
                proccess.quantum = (rand()%proccess.execution_time_needed)+1;
            else
                proccess.quantum = pow(2, level);
            proccess.level = level;
            proccess.state = STATE_READY;
            next_p_id++;
            proccess_queue[level][last_p[level]] = proccess;
            printf("%s%lu prozesua sortuta.\n", KCYN, proccess_queue[level][last_p[level]].id);
            last_p[level] = (last_p[level]+1)%PROC_KOP_MAX;
        }
        else
            printf("%sProzesu kopuru maximoa prozesuaren mailan. Ezin da prozesurik sortu.\n", KCYN);
        pthread_mutex_unlock(&mutex_proccess_queue);
    } // while (1) bukaera
    return 0;
}