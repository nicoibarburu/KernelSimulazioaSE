#include <pthread.h>
#include <unistd.h>
#include "kernel.h"

void *erlojua(void *ten_kop) {
    int tenp_kop = *((int*)ten_kop);
    done = 0;
    while(1){
        pthread_mutex_lock(&mutex);
        while(done < tenp_kop)
            pthread_cond_wait(&cond, &mutex);
        usleep (1e6/frequence);
        done = 0;
        pthread_cond_broadcast(&cond2);
        pthread_mutex_unlock(&mutex);
    }
}