#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include "kernel.h"

void *prozesu_exekutatzailea() {
    thread *now_thread;
    int i, j, k, l, level;
    pthread_mutex_lock(&mutex_ep);
    while (1) {
        pthread_cond_wait(&cond_ep, &mutex_ep);
        for (i=0; i<cpus.cpu_quant; i++)
        for (j=0; j<cpus.core_quant; j++)
        for (k=0; k<cpus.thread_quant; k++) {
            now_thread = &cpus.cct[i][j][k];
            pthread_mutex_lock(&now_thread->mutex_e);
            if (now_thread->executing.state == STATE_EXECUTION) {
                now_thread->exec_time++;
                now_thread->executing.time_executed++;
                if (now_thread->exec_time-1 == 0)
                    printf("%s%lu haria %lu prozesua exekutatzen hasi da...\n", KRED, now_thread->tid, now_thread->executing.id);
                else if (now_thread->executing.time_executed == now_thread->executing.execution_time_needed) {
                    now_thread->executing.state = STATE_FINISHED;
                    printf("%s%lu hariak %lu prozesua exekutatzen amaitu du.\n", KRED, now_thread->tid, now_thread->executing.id);
                    now_thread->executing = null_proccess;
                    now_thread->free = true;
                }
                else if (now_thread->exec_time == now_thread->executing.quantum) {
                    now_thread->executing.state = STATE_BLOCKED;
                    printf("%s%lu harian %lu prozesuaren quantum-a agortu da.\n", KRED, now_thread->tid, now_thread->executing.id);
                }
                else
                    printf("%s%lu haria %lu prozesua exekutatzen...\n", KRED, now_thread->tid, now_thread->executing.id);
            }
            else {
                printf("%s%lu harian ez dago exekutatzeko prozesurik.\n", KRED, now_thread->tid);
            }
            pthread_mutex_unlock(&now_thread->mutex_e);
        }
    }
}