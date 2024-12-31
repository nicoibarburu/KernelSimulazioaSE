#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <math.h>
#include "kernel.h"

void *scheduler_dispatcher() {
    thread *next_t_exec, *lag = (thread *)malloc(sizeof(thread));
    int i, level, out_level, cct[3];
    unsigned int out_exec_time;
    pthread_mutex_lock(&mutex_sd);
    while(1) {
        pthread_cond_wait(&cond_sd, &mutex_sd);
        if (finish)
            break;
        printf("%sSecheduler-a eta dispatcher-a aktibatuta.\n", KYEL);
        printf("%sBlokeatutako prozesuak prozesu ilarara sartzen...\n", KYEL);
        cct[0] = 0;
        cct[1] = 0;
        cct[2] = -1;
        next_free_ocup_cct(cct, false);
        while (cct[0] != -1) {
            if (cpus.cct[cct[0]][cct[1]][cct[2]].executing.state == STATE_BLOCKED) {
                next_t_exec = &cpus.cct[cct[0]][cct[1]][cct[2]];
                if (scheduler_politic == SCHEDULER_POLITIC_LDDQ) {
                    pthread_mutex_lock(&next_t_exec->mutex_e);
                    if (next_t_exec->executing.level < PRIORITY_LEVELS-1) {
                        next_t_exec->executing.level--;
                        next_t_exec->executing.quantum = pow(2, next_t_exec->executing.level);
                    }
                    pthread_mutex_unlock(&next_t_exec->mutex_e);
                }
                pthread_mutex_lock(&mutex_proccess_queue);
                if (last_p[next_t_exec->executing.level] != first_p[next_t_exec->executing.level] ||
                  proccess_queue[next_t_exec->executing.level][first_p[next_t_exec->executing.level]].state == STATE_UNDEFINED) {
                    proccess_queue[next_t_exec->executing.level][last_p[next_t_exec->executing.level]] = next_t_exec->executing;
                    proccess_queue[next_t_exec->executing.level][last_p[next_t_exec->executing.level]].state = STATE_READY;
                    last_p[next_t_exec->executing.level] = (last_p[next_t_exec->executing.level]+1)%PROC_KOP_MAX;
                    pthread_mutex_unlock(&mutex_proccess_queue);
                    printf("%s%lu prozesua prozesu ilarara sartuta.\n", KYEL, next_t_exec->executing.id);
                    pthread_mutex_lock(&next_t_exec->mutex_e);
                    next_t_exec->executing = null_proccess;
                    next_t_exec->free = true;
                    next_t_exec->exec_time = 0;
                    pthread_mutex_unlock(&next_t_exec->mutex_e);
                }
                else {
                    pthread_mutex_unlock(&mutex_proccess_queue);
                    pthread_mutex_lock(&next_t_exec->mutex_e);
                    next_t_exec->executing.level = 0;
                    next_t_exec->executing.quantum = next_t_exec->executing.execution_time_needed;
                    next_t_exec->executing.state = STATE_EXECUTION;
                    next_t_exec->exec_time = 0;
                    pthread_mutex_unlock(&next_t_exec->mutex_e);
                }
            } // state == state_blocked bukaera
            next_free_ocup_cct(cct, false);
        } // while (cct[0] != -1) bukaera
        cct[0] = 0;
        cct[1] = 0;
        cct[2] = -1;
        next_t_exec = &null_thread;
        if (scheduler_politic == SCHEDULER_POLITIC_RORO) {
            level = 0;
            if (proccess_queue[level][first_p[level]].state == STATE_READY) {
                printf("%s%lu prozesua exekutatzeko haria bilatzen...\n", KYEL, proccess_queue[level][first_p[level]].id);
                next_free_ocup_cct(cct, true);
                if (cct[0] != -1) {
                    next_t_exec = &cpus.cct[cct[0]][cct[1]][cct[2]];
                    printf("%s%lu prozesua exekutatzen jarriko da %lu harian.\n", KYEL, proccess_queue[level][first_p[level]].id, next_t_exec->tid);
                    pthread_mutex_lock(&mutex_proccess_queue);
                    lag->executing = proccess_queue[level][first_p[level]];
                    proccess_queue[level][first_p[level]] = null_proccess;
                    first_p[level] = (first_p[level]+1)%PROC_KOP_MAX;
                    pthread_mutex_unlock(&mutex_proccess_queue);
                    pthread_mutex_lock(&next_t_exec->mutex_e);
                    next_t_exec->executing = lag->executing;
                    next_t_exec->executing.state = STATE_EXECUTION;
                    next_t_exec->free = false;
                    pthread_mutex_unlock(&next_t_exec->mutex_e);
                }
                else
                    printf("%sHari guztiak okupatuta daude\n", KYEL);
            } // state == state_ready bukaera
            else
                printf("%sEz dago martxan jartzeko prozesurik.\n", KYEL);
        } // scheduler_politic == RORO bukaera
        else if (scheduler_politic == SCHEDULER_POLITIC_LDDQ) {
            for (level=0; level<PRIORITY_LEVELS; level++) {
                if (proccess_queue[level][first_p[level]].state == STATE_READY)
                    break;
            }
            if (proccess_queue[level][first_p[level]].state == STATE_READY) {
                printf("%s%lu prozesua exekutatzeko haria bilatzen...\n", KYEL, proccess_queue[level][first_p[level]].id);
                next_free_ocup_cct(cct, true);
                if (cct[0] == -1) {
                    cct[0] = 0;
                    cct[1] = 0;
                    cct[2] = -1;
                    next_free_ocup_cct(cct, false);
                    out_level = level;
                    out_exec_time = proccess_queue[level][first_p[level]].time_executed;
                    while (cct[0] != -1) {
                        lag = &cpus.cct[cct[0]][cct[1]][cct[2]];
                        pthread_mutex_lock(&lag->mutex_e);
                        if (lag->executing.level >= out_level && lag->executing.time_executed > out_exec_time) {
                            out_level = lag->executing.level;
                            out_exec_time = lag->executing.time_executed;
                            next_t_exec = lag;
                        }
                        pthread_mutex_unlock(&lag->mutex_e);
                        next_free_ocup_cct(cct, false);
                    }
                    if (next_t_exec->tid != 0) {
                        pthread_mutex_lock(&next_t_exec->mutex_e);
                        next_t_exec->executing.state = STATE_READY;
                        if (next_t_exec->executing.level < PRIORITY_LEVELS-1) {
                            next_t_exec->executing.level--;
                            next_t_exec->executing.quantum = pow(2, next_t_exec->executing.level);
                        }
                        pthread_mutex_unlock(&next_t_exec->mutex_e);
                        pthread_mutex_lock(&mutex_proccess_queue);
                        if (last_p[next_t_exec->executing.level] != first_p[next_t_exec->executing.level] ||
                          proccess_queue[next_t_exec->executing.level][first_p[next_t_exec->executing.level]].state == STATE_UNDEFINED) {
                            proccess_queue[next_t_exec->executing.level][last_p[next_t_exec->executing.level]] = next_t_exec->executing;
                            last_p[next_t_exec->executing.level] = (last_p[next_t_exec->executing.level]+1)%PROC_KOP_MAX;
                            pthread_mutex_unlock(&mutex_proccess_queue);
                        }
                        else {
                            pthread_mutex_unlock(&mutex_proccess_queue);
                            pthread_mutex_lock(&next_t_exec->mutex_e);
                            next_t_exec->executing.level = 0;
                            next_t_exec->executing.quantum = next_t_exec->executing.execution_time_needed;
                            next_t_exec->executing.state = STATE_EXECUTION;
                            next_t_exec->exec_time = 0;
                            pthread_mutex_unlock(&next_t_exec->mutex_e);
                            next_t_exec = &null_thread;
                        }
                    } // next_t_exec->tid != 0 bukaera
                } // cct[0] == -1 bukaera
                else
                    next_t_exec = &cpus.cct[cct[0]][cct[1]][cct[2]];
                if (next_t_exec->tid != 0) {
                    printf("%s%lu prozesua exekutatzen jarriko da %lu harian.\n", KYEL, proccess_queue[level][first_p[level]].id, next_t_exec->tid);
                    pthread_mutex_lock(&next_t_exec->mutex_e);
                    next_t_exec->executing = proccess_queue[level][first_p[level]];
                    next_t_exec->executing.state = STATE_EXECUTION;
                    next_t_exec->free = false;
                    next_t_exec->exec_time = 0;
                    pthread_mutex_unlock(&next_t_exec->mutex_e);
                    pthread_mutex_lock(&mutex_proccess_queue);
                    proccess_queue[level][first_p[level]] = null_proccess;
                    first_p[level] = (first_p[level]+1)%PROC_KOP_MAX;
                    pthread_mutex_unlock(&mutex_proccess_queue);
                } // next_t_exec->tid != 0 bukaera
                else
                    printf("%sHari guztiak ataza garrantzitsuagoekin okupatuta daude.\n", KYEL);
            } // state == state_ready bukaera
            else
                printf("%sEz dago martxan jartzeko prozesurik.\n", KYEL);
        } // scheduler_politic == LDDQ bukaera
    } // while(1) bukaera
    free(lag);
    return 0;
}