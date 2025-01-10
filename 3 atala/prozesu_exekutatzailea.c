#include "kernel.h"

void *prozesu_exekutatzailea() {
    thread *now_thread;
    int i, j, k, l, level;
    pthread_mutex_lock(&mutex_ep);
    while (1) {
        pthread_cond_wait(&cond_ep, &mutex_ep);
        if (finish)
            break;
        for (i=0; i<cpus.cpu_quant; i++)
        for (j=0; j<cpus.core_quant; j++)
        for (k=0; k<cpus.thread_quant; k++) {
            
        }
    } // while (1) bukaera
    return 0;
}