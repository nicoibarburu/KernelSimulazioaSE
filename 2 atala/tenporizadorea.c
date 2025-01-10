#include "kernel.h"

void *tenporizadorea() {
    uint64_t tick_kont = 0, seg_kont = 0;
    pthread_mutex_lock(&mutex);
    while(1) {
        done++;
        tick_kont++;
        printf("\n%s%lu tick\n", KGRN, tick_kont);
        if (tick_kont%frequence == 0) {
            seg_kont++;
            printf("%s%lu segundu pasa dira.\n", KGRN, seg_kont);
            pthread_cond_signal(&cond_ep);
            if (seg_kont%timer_ps == 0)
                pthread_cond_signal(&cond_ps);
            if (seg_kont%timer_sd == 0)
                pthread_cond_signal(&cond_sd);
        }
        pthread_cond_signal(&cond);
        pthread_cond_wait(&cond2, &mutex);
        if (finish)
            break;
    }
    return 0;
}