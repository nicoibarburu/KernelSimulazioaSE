#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>
#include "kernel.h"

void *prozesu_exekutatzailea(void *tidv) {
    PCB proccess = null_proccess;
    int i, level, tid_ep;
    double exec_time;
    tid_ep = *((int*)tidv);
    pthread_mutex_lock(&mutex_ep);
    while (1) {
        if (proccess.state == STATE_READY && (last_p[proccess.level] != first_p[proccess.level] || proccess_queue[proccess.level][first_p[level]].state == STATE_UNDEFINED)) {
            pthread_mutex_lock(&mutex_proccess_queue);
            if (last_p[proccess.level] != first_p[proccess.level] || proccess_queue[proccess.level][first_p[level]].state == STATE_UNDEFINED) {
                proccess_queue[proccess.level][last_p[proccess.level]] = proccess;
                last_p[proccess.level] = (last_p[proccess.level]+1)%PROC_KOP_MAX;
                pthread_mutex_unlock(&mutex_proccess_queue);
                pthread_mutex_lock(&mutex_executing[tid_ep]);
                if (proccess.id != executing[tid_ep].id) {
                    proccess = executing[tid_ep];
                    pthread_mutex_unlock(&mutex_executing[tid_ep]);
                    exec_time = 0;
                    printf("%s%lu prozesua exekutatzen hasita...\n", KRED, proccess.id);
                }
                else {
                    executing[tid_ep] = null_proccess;
                    pthread_mutex_unlock(&mutex_executing[tid_ep]);
                    proccess = null_proccess;
                }
            }
            else
              pthread_mutex_unlock(&mutex_proccess_queue);
            
        }
        if (proccess.state == STATE_READY)
            continue;
        pthread_cond_wait(&cond_ep, &mutex_ep);
        pthread_mutex_lock(&mutex_executing[tid_ep]);
        if (proccess.id != executing[tid_ep].id) {
            if (proccess.state == STATE_EXECUTION) {
                proccess.state = STATE_READY;
                if (proccess.level < PRIORITY_LEVELS-1) {
                    proccess.level = proccess.level+1;
                    proccess.quantum = pow(2, proccess.level);
                }
            }
            else {
                proccess = executing[tid_ep];
                pthread_mutex_unlock(&mutex_executing[tid_ep]);
                exec_time = 0;
                printf("%s%lu prozesua exekutatzen hasita...\n", KRED, proccess.id);
            }
        }   
        else if (proccess.id != null_proccess.id) {
            pthread_mutex_unlock(&mutex_executing[tid_ep]);
            exec_time++;
            proccess.time_executed++;
            if (proccess.time_executed == proccess.execution_time_needed) {
                proccess.state = STATE_FINISHED;
                printf("%s%lu prozesua amaitu da.\n", KRED, proccess.id);
                proccess = null_proccess;
                pthread_mutex_lock(&mutex_executing[tid_ep]);
                executing[tid_ep] = null_proccess;
                pthread_mutex_unlock(&mutex_executing[tid_ep]);
            }
            else if (exec_time == proccess.quantum) {
                proccess.state = STATE_READY;
                if (proccess.level < PRIORITY_LEVELS-1) {
                    proccess.level = proccess.level+1;
                    proccess.quantum = pow(2, proccess.level);
                }
                printf("%s%lu prozesuaren quantum-a agortu da.\n", KRED, proccess.id);
            }
            else
                printf("%s%lu prozesua exekutatzen...\n", KRED, proccess.id);
        }
        else
            pthread_mutex_unlock(&mutex_executing[tid_ep]);
    }
}