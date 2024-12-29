#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>
#include "kernel.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER, mutex_ps = PTHREAD_MUTEX_INITIALIZER, mutex_sd = PTHREAD_MUTEX_INITIALIZER,
    mutex_ep = PTHREAD_MUTEX_INITIALIZER, mutex_proccess_queue = PTHREAD_MUTEX_INITIALIZER, *mutex_executing;
pthread_cond_t cond, cond2, cond_ps, cond_sd, cond_ep;
PCB proccess_queue[PRIORITY_LEVELS][PROC_KOP_MAX], *executing, null_proccess;
CPU cpus;
unsigned long next_p_id;
unsigned int first_p[PRIORITY_LEVELS], last_p[PRIORITY_LEVELS], done, timer_ps, timer_sd, frequence;
char scheduler_politic;

void next_free_ocup_cct(int nextcct[3], bool next_free) {
    int i, j, k;
    for (i=0; i<3; i++) {
        nextcct[i] = -1;
    }
    for (i=0; i<cpus.cpu_quant; i++)
    for (j=0; j<cpus.core_quant; j++)
    for (k=0; k<cpus.thread_quant; k++) {
        if (next_free && cpus.cct[i][j][k].free) {
            nextcct[0] = i;
            nextcct[1] = j;
            nextcct[2] = k;
            break;
        }
        else if (!next_free && !cpus.cct[i][j][k].free) {
            nextcct[0] = i;
            nextcct[1] = j;
            nextcct[2] = k;
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 8) {
        printf("%sErabilera: %s erlojuaren_maiztasuna(Hz) zenbat_segunduro_prozesua_sortu zenbat_segunduro_schedulerra_aktibatu\
         CPU_kant CPU-ko_core_kant core-ko_hari_kant schedule(Round Robin: %c edo Lehentasunekin: %c)\n", KNRM, argv[0], SCHEDULER_POLITIC_RORO, SCHEDULER_POLITIC_LDDQ);
        exit(1);
    }
    if (sscanf(argv[1], "%i", &frequence) != 1 || frequence < 1) {
        printf("%sErlojuaren maiztasunak zenbaki oso positiboa izan behar du (0 balioa izan ezik).\n", KNRM);
        exit(2);
    }
    if (sscanf(argv[2], "%i", &timer_ps) != 1 || timer_ps < 1) {
        printf("%sTenporizadoreak prozesu sortzailea zenbat segunduro deituko duen zenbaki oso positiboa izan behar du (0 balioa izan ezik).\n", KNRM);
        exit(2);
    }
    if (sscanf(argv[3], "%i", &timer_sd) != 1 || timer_ps < 1) {
        printf("%sTenporizadoreak sheduler-a zenbat segunduro deituko duen zenbaki oso positiboa izan behar du (0 balioa izan ezik).\n", KNRM);
        exit(2);
    }
    if (sscanf(argv[4], "%hu", &cpus.cpu_quant) != 1 || cpus.cpu_quant < 1 || cpus.cpu_quant > MAX_CPU_QUANT) {
        printf("%sCPU kantitateak 1-4 tarteko zenbaki osoa izan behar du.\n", KNRM);
        exit(2);
    }
    if (sscanf(argv[5], "%hu", &cpus.core_quant) != 1 || cpus.core_quant < 1 || cpus.core_quant > MAX_CORE_QUANT) {
        printf("%sCPU-ko core kantitateak 1-8 tarteko zenbaki osoa izan behar du.\n", KNRM);
        exit(2);
    }
    if (sscanf(argv[6], "%hu", &cpus.thread_quant) != 1 || cpus.thread_quant < 1 || cpus.thread_quant > MAX_THREAD_QUANT) {
        printf("%sCore-ko hari kantitateak 1-16 tarteko zenbaki osoa izan behar du.\n", KNRM);
        exit(2);
    }
    if (*argv[7] != SCHEDULER_POLITIC_RORO && *argv[7] != SCHEDULER_POLITIC_LDDQ) {
        printf("%sScheduler-a RORO edo DPDQ izan behar da.\n", KNRM);
        exit(2);
    }
    scheduler_politic = *argv[7];

    if (cpus.cpu_quant*cpus.core_quant*cpus.thread_quant<5) {
        printf("%sEzin dira oinarrizko hariak sortu. Hari gehiago behar dira.\n", KNRM);
        exit(3);
    }

    int i, j, k, nextcct[3];
    int tenp_kop = 1;
    null_proccess.id = 0;
    null_proccess.execution_time_needed = 0;
    null_proccess.time_executed = 0;
    null_proccess.quantum = 0;
    null_proccess.level = PRIORITY_LEVELS-1;
    null_proccess.state = STATE_UNDEFINED;
    next_p_id = 1;
    for (i=0; i<cpus.cpu_quant; i++)
    for (j=0; j<cpus.core_quant; j++)
    for (k=0; k<cpus.thread_quant; k++)
        cpus.cct[i][j][k].free = true;
    for (i=0; i<PRIORITY_LEVELS; i++) {
        first_p[i] = 0;
        last_p[i] = 0;
        for (j=0; j<PROC_KOP_MAX; j++)
            proccess_queue[i][j] = null_proccess;
    }
    executing = (PCB*)malloc((cpus.cpu_quant*cpus.core_quant*cpus.thread_quant-4)*sizeof(PCB));
    mutex_executing = (pthread_mutex_t*)malloc((cpus.cpu_quant*cpus.core_quant*cpus.thread_quant-4)*sizeof(pthread_mutex_t));
    for (i=0; i<cpus.cpu_quant*cpus.core_quant*cpus.thread_quant-4; i++) {
        executing[i] = null_proccess;
        pthread_mutex_init(&mutex_executing[i], NULL);
    }
    
    next_free_ocup_cct(nextcct, true);
    if (pthread_create(&cpus.cct[nextcct[0]][nextcct[1]][nextcct[2]].tid, NULL, prozesu_sortzailea, NULL) != 0) {
        printf("%sErrorea prozesu sortzailea martxan jartzean.\n", KNRM);
        exit(4);
    }
    cpus.cct[nextcct[0]][nextcct[1]][nextcct[2]].free = false;
    printf("%sProzesu sortzailea martxan jarri da.\n", KNRM);

    next_free_ocup_cct(nextcct, true);
    if (pthread_create(&cpus.cct[nextcct[0]][nextcct[1]][nextcct[2]].tid, NULL, scheduler_dispatcher, NULL) != 0) {
        printf("%sErrorea scheduler-a martxan jartzean.\n", KNRM);
        exit(4);
    }
    cpus.cct[nextcct[0]][nextcct[1]][nextcct[2]].free = false;
    printf("%sScheduler-a martxan jarri da.\n", KNRM);

    next_free_ocup_cct(nextcct, true);
    if (pthread_create(&cpus.cct[nextcct[0]][nextcct[1]][nextcct[2]].tid, NULL, erlojua, &tenp_kop) != 0) {
        printf("%sErrorea erlojua martxan jartzean.\n", KNRM);
        exit(4);
    }
    cpus.cct[nextcct[0]][nextcct[1]][nextcct[2]].free = false;
    printf("%sErlojua martxan jarri da.\n", KNRM);

    next_free_ocup_cct(nextcct, true);
    if (pthread_create(&cpus.cct[nextcct[0]][nextcct[1]][nextcct[2]].tid, NULL, tenporizadorea, NULL) != 0) {
        printf("%sErrorea tenporizadorea martxan jartzean.\n", KNRM);
        exit(4);
    }
    cpus.cct[nextcct[0]][nextcct[1]][nextcct[2]].free = false;
    printf("%sTenporizadorea martxan jarri da.\n", KNRM);

    for (i=0; i<cpus.cpu_quant*cpus.core_quant*cpus.thread_quant-4; i++) {
        int *tid = (int*)malloc(sizeof(int));
        *tid = i;
        next_free_ocup_cct(nextcct, true);
        if (pthread_create(&cpus.cct[nextcct[0]][nextcct[1]][nextcct[2]].tid, NULL, prozesu_exekutatzailea, tid) != 0) {
            printf("%sErrorea prozesu exekutadoreak martxan jartzean.\n", KNRM);
            exit(4);
        }
        cpus.cct[nextcct[0]][nextcct[1]][nextcct[2]].free = false;
        printf("%sProzesu exekutatzailea %d martxan jartzen...\n", KNRM, i);
    }

    for (i=0; i<cpus.cpu_quant*cpus.core_quant*cpus.thread_quant; i++) {
        next_free_ocup_cct(nextcct, false);
        pthread_join(cpus.cct[nextcct[0]][nextcct[1]][nextcct[2]].tid, NULL);
    }

    free(executing);
    free(mutex_executing);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutex_ps);
    pthread_mutex_destroy(&mutex_sd);
    pthread_mutex_destroy(&mutex_ep);
    pthread_mutex_destroy(&mutex_proccess_queue);
    for (i=0; i<cpus.cpu_quant*cpus.core_quant*cpus.thread_quant-4; i++)
        pthread_mutex_destroy(&mutex_executing[i]);
    pthread_cond_destroy(&cond);
    pthread_cond_destroy(&cond2);
    pthread_cond_destroy(&cond_ps);
    pthread_cond_destroy(&cond_sd);
    pthread_cond_destroy(&cond_ep);
    return 0;
}