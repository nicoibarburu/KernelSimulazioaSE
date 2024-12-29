#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define PROC_KOP_MAX 100

typedef struct {
    unsigned long id;
} PCB;

struct CPUs {
    int cpu_quant;
    int core_quant;
    int thread_quant;
} cpus;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER, mutex_ps = PTHREAD_MUTEX_INITIALIZER, mutex_sd = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond, cond2, cond_ps, cond_sd;
PCB proccess_queue[PROC_KOP_MAX];
unsigned long next_p_id, clock_th, timer_th, scheduler_th, proccess_cr_th;
int done, timer_ps, timer_sd, frequence;
unsigned int first_p, last_p;

void* erlojua(void* ten_kop) {
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

void* tenporizadorea() {
    long unsigned int tick_kont = 0, seg_kont = 0;
    pthread_mutex_lock(&mutex);
    while(1) {
        done++;
        tick_kont++;
        printf("\n%lu tick\n", tick_kont);
        if (tick_kont%frequence == 0) {
            seg_kont++;
            printf("%lu segundu pasa dira.\n", seg_kont);
        }
        if (tick_kont%frequence == 0 && seg_kont%timer_ps == 0) {
            pthread_cond_signal(&cond_ps);
        }
        if (tick_kont%frequence == 0 && seg_kont%timer_sd == 0) {
            pthread_cond_signal(&cond_sd);
        }
        pthread_cond_signal(&cond);
        pthread_cond_wait(&cond2, &mutex);
    }
}

void* prozesu_sortzailea() {
    pthread_mutex_lock(&mutex_ps);
    while(1) {
        pthread_cond_wait(&cond_ps, &mutex_ps);
        if ((last_p+1)%PROC_KOP_MAX!=first_p) {
            last_p = (last_p+1)%PROC_KOP_MAX;
            proccess_queue[last_p].id = next_p_id;
            printf("%lu prozesua sortuta.\n", proccess_queue[last_p].id);
            next_p_id++;
        }
        else
            printf("Prozesu kopuru maximoa. Ezin da prozesurik sortu.\n");
    }
}

void* scheduler_dispatcher() {
    pthread_mutex_lock(&mutex_sd);
    while(1) {
        pthread_cond_wait(&cond_sd, &mutex_sd);
        printf("Secheduler-a aktibatuta.\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        printf("Erabilera: %s erlojuaren_maiztasuna(Hz) zenbat_segunduro_prozesua_sortu zenbat_segunduro_schedulerra_aktibatu CPU_kant CPU-ko_core_kant core-ko_hari_kant\n", argv[0]);
        exit(1);
    }
    if (sscanf(argv[1], "%i", &frequence) != 1) {
        printf("Erlojuaren maiztasunak zenbaki osoa izan behar du.\n");
        exit(2);
    }
    if (sscanf(argv[2], "%i", &timer_ps) != 1) {
        printf("Tenporizadoreak prozesu sortzailea zenbat segunduro deituko duen zenbaki osoa izan behar du.\n");
        exit(2);
    }
    if (sscanf(argv[3], "%i", &timer_sd) != 1) {
        printf("Tenporizadoreak sheduler-a zenbat segunduro deituko duen zenbaki osoa izan behar du.\n");
        exit(2);
    }
    if (sscanf(argv[4], "%i", &cpus.cpu_quant) != 1) {
        printf("CPU kantitateak zenbaki osoa izan behar du.\n");
        exit(2);
    }
    if (sscanf(argv[5], "%i", &cpus.core_quant) != 1) {
        printf("CPU-ko core kantitateak zenbaki osoa izan behar du.\n");
        exit(2);
    }
    if (sscanf(argv[6], "%i", &cpus.thread_quant) != 1) {
        printf("Core-ko hari kantitateak zenbaki osoa izan behar du.\n");
        exit(2);
    }
    cpus.core_quant = cpus.cpu_quant*cpus.core_quant;
    cpus.thread_quant = cpus.core_quant*cpus.thread_quant;
    if (cpus.thread_quant<4) {
        printf("Ezin dira oinarrizko hariak sortu. Hari gehiago behar dira.\n");
        exit(3);
    }
    next_p_id = 0;
    first_p = 0;
    last_p = 0;
    int tenp_kop = 1;
    if (pthread_create(&proccess_cr_th, NULL, prozesu_sortzailea, NULL) != 0) {
        printf("Errorea prozesu sortzailea martxan jartzean.\n");
        exit(4);
    }
    printf("Prozesu sortzailea martxan jarri da.\n");

    if (pthread_create(&scheduler_th, NULL, scheduler_dispatcher, NULL) != 0) {
        printf("Errorea scheduler-a martxan jartzean.\n");
        exit(4);
    }
    printf("Scheduler-a martxan jarri da.\n");
    
    if (pthread_create(&clock_th, NULL, erlojua, &tenp_kop) != 0) {
        printf("Errorea erlojua martxan jartzean.\n");
        exit(4);
    }
    printf("Erlojua martxan jarri da.\n");
    
    if (pthread_create(&timer_th, NULL, tenporizadorea, NULL) != 0) {
        printf("Errorea tenporizadorea martxan jartzean.\n");
        exit(4);
    }
    printf("Tenporizadorea martxan jarri da.\n");

    pthread_join(proccess_cr_th, NULL);
    pthread_join(scheduler_th, NULL);
    pthread_join(clock_th, NULL);
    pthread_join(timer_th, NULL);

}