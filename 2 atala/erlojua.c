#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include "kernel.h"

void *erlojua(void *tenp_k) {
    int tenp_kop = *(int *)tenp_k;
    done = 0;
    while(1){
        pthread_mutex_lock(&mutex);
        while(done < tenp_kop)
            pthread_cond_wait(&cond, &mutex);
        usleep (1e6/frequence);
        done = 0;
        pthread_cond_broadcast(&cond2);
        pthread_mutex_unlock(&mutex);
        if (finish)
            break;
    }
    return 0;
}