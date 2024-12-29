#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "kernel.h"

void *scheduler_dispatcher() {
    int i, level, out_level, next_t_exec = -1;
    double out_exec_time;
    pthread_mutex_lock(&mutex_sd);
    while(1) {
        pthread_cond_wait(&cond_sd, &mutex_sd);
        printf("%sSecheduler-a aktibatuta.\n", KYEL);
        //if (scheduler_politic == SCHEDULER_POLITIC_RORO) {
        if (scheduler_politic == SCHEDULER_POLITIC_RORO) {
            level = 0;
            if (proccess_queue[level][first_p[level]].state == STATE_READY) {
                for (i=0; i<cpus.cpu_quant*cpus.core_quant*cpus.thread_quant-4; i++) {
                    if (executing[i].state == STATE_UNDEFINED) {
                        next_t_exec = i;
                        break;
                    }
                }
                if (next_t_exec != -1) {
                    printf("%s%lu prozesua exekutatzen jarriko da.\n", KYEL, proccess_queue[level][first_p[level]].id);
                    pthread_mutex_lock(&mutex_executing[next_t_exec]);
                    executing[next_t_exec] = proccess_queue[level][first_p[level]];
                    executing[next_t_exec].state = STATE_EXECUTION;
                    pthread_mutex_unlock(&mutex_executing[next_t_exec]);
                    pthread_mutex_lock(&mutex_proccess_queue);
                    proccess_queue[level][first_p[level]] = null_proccess;
                    first_p[level] = (first_p[level]+1)%PROC_KOP_MAX;
                    pthread_mutex_unlock(&mutex_proccess_queue);
                    next_t_exec = -1;
                }
                else
                    printf("%sHari guztiak okupatuta daude\n", KYEL);
            }
            else
                printf("%sEz dago martxan jartzeko prozesurik.\n", KYEL);
        } // scheduler_politic == SCHEDULER_POLITIC_RORO bukaera
        //else if (scheduler_politic == SCHEDULER_POLITIC_DPDQ) {
        if (scheduler_politic == SCHEDULER_POLITIC_LDDQ) {
            for (level=0; level<PRIORITY_LEVELS; level++) {
                if (proccess_queue[level][first_p[level]].state == STATE_READY)
                    break;
            }
            if (proccess_queue[level][first_p[level]].state == STATE_READY) {
                for (i=0; i<cpus.cpu_quant*cpus.core_quant*cpus.thread_quant-4; i++) {
                    if (executing[i].state == STATE_UNDEFINED) {
                        next_t_exec = i;
                        break;
                    }
                }
                if (next_t_exec == -1) {
                    out_level = PRIORITY_LEVELS;
                    out_exec_time = 0;
                    for (i=0; i<cpus.cpu_quant*cpus.core_quant*cpus.thread_quant-4; i++) {
                        pthread_mutex_lock(&mutex_executing[i]);
                        if (executing[i].level < out_level && executing[i].level >= proccess_queue[level][first_p[level]].level) {
                            if (executing[i].time_executed > out_exec_time && executing[i].time_executed > proccess_queue[level][first_p[level]].time_executed) {
                                out_level = executing[i].level;
                                out_exec_time = executing[i].time_executed;
                                next_t_exec = i;
                            }
                        }
                        pthread_mutex_unlock(&mutex_executing[i]);
                    }
                }
                if (next_t_exec != -1) {
                    printf("%s%lu prozesua exekutatzen jarriko da.\n", KYEL, proccess_queue[level][first_p[level]].id);
                    pthread_mutex_lock(&mutex_executing[next_t_exec]);
                    executing[next_t_exec] = proccess_queue[level][first_p[level]];
                    executing[next_t_exec].state = STATE_EXECUTION;
                    pthread_mutex_unlock(&mutex_executing[next_t_exec]);
                    pthread_mutex_lock(&mutex_proccess_queue);
                    proccess_queue[level][first_p[level]] = null_proccess;
                    first_p[level] = (first_p[level]+1)%PROC_KOP_MAX;
                    pthread_mutex_unlock(&mutex_proccess_queue);
                    next_t_exec = -1;
                }
                else
                    printf("%sHari guztiak ataza garrantzitsuagoekin okupatuta daude.\n", KYEL);
            } // exekutatzeko prest dagoen prozesu bat dago bukaera
            else
                printf("%sEz dago martxan jartzeko prozesurik.\n", KYEL);
        } // scheduler_politic == SCHEDULER_POLITIC_DPDQ bukaera
    } // while(1) bukaera
}