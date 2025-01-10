#include "kernel.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER, mutex_pk = PTHREAD_MUTEX_INITIALIZER, mutex_sd = PTHREAD_MUTEX_INITIALIZER,
    mutex_ep = PTHREAD_MUTEX_INITIALIZER, mutex_proccess_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond, cond2, cond_pk, cond_sd, cond_ep;
PCB proccess_queue[PRIORITY_LEVELS][PROC_KOP_MAX], null_proccess;
CPU cpus;
thread null_thread;
physical_memory memory;
uint64_t next_p_id, erlojua_tid, tenporizadorea_tid, prozesu_kargatzailea_tid, scheduler_dispatcher_tid, prozesu_exekutatzailea_tid;
uint32_t first_p[PRIORITY_LEVELS], last_p[PRIORITY_LEVELS], done, timer_pk, timer_sd, frequence, next_Text, next_Data, next_pageT;
char scheduler_politic;
bool finish;

void next_free_ocup_cct(int cct[3], bool free) {
    int i, j, k, nextcct[3] = {-1, -1, -1};
    for (i=0; i<cpus.cpu_quant; i++) {
        for (j=0; j<cpus.core_quant; j++) {
            for (k=0; k<cpus.thread_quant; k++) {
                if (free && cpus.cct[i][j][k].free && (k > cct[2] || j > cct[1] || i > cct[0])) {
                    nextcct[0] = i;
                    nextcct[1] = j;
                    nextcct[2] = k;
                    break;
                }
                else if (!free && !cpus.cct[i][j][k].free && (k > cct[2] || j > cct[1] || i > cct[0])) {
                    nextcct[0] = i;
                    nextcct[1] = j;
                    nextcct[2] = k;
                    break;
                }
            }
            if (nextcct[0] != -1)
                break;
        }
        if (nextcct[0] != -1)
                break;
    }
    cct[0] = nextcct[0];
    cct[1] = nextcct[1];
    cct[2] = nextcct[2];
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
    if (sscanf(argv[2], "%i", &timer_pk) != 1 || timer_pk < 1) {
        printf("%sTenporizadoreak prozesu kargatzailea zenbat segunduro deituko duen zenbaki oso positiboa izan behar du (0 balioa izan ezik).\n", KNRM);
        exit(2);
    }
    if (sscanf(argv[3], "%i", &timer_sd) != 1 || timer_pk < 1) {
        printf("%sTenporizadoreak sheduler-a zenbat segunduro deituko duen zenbaki oso positiboa izan behar du (0 balioa izan ezik).\n", KNRM);
        exit(2);
    }
    if (sscanf(argv[4], "%hhu", &cpus.cpu_quant) != 1 || cpus.cpu_quant < 1 || cpus.cpu_quant > MAX_CPU_QUANT) {
        printf("%sCPU kantitateak 1-4 tarteko zenbaki osoa izan behar du.\n", KNRM);
        exit(2);
    }
    if (sscanf(argv[5], "%hhu", &cpus.core_quant) != 1 || cpus.core_quant < 1 || cpus.core_quant > MAX_CORE_QUANT) {
        printf("%sCPU-ko core kantitateak 1-8 tarteko zenbaki osoa izan behar du.\n", KNRM);
        exit(2);
    }
    if (sscanf(argv[6], "%hhu", &cpus.thread_quant) != 1 || cpus.thread_quant < 1 || cpus.thread_quant > MAX_THREAD_QUANT) {
        printf("%sCore-ko hari kantitateak 1-16 tarteko zenbaki osoa izan behar du.\n", KNRM);
        exit(2);
    }
    if (*argv[7] != SCHEDULER_POLITIC_RORO && *argv[7] != SCHEDULER_POLITIC_LDDQ) {
        printf("%sScheduler-a RORO edo DPDQ izan behar da.\n", KNRM);
        exit(2);
    }
    scheduler_politic = *argv[7];

    int i, j, k;
    char fin;
    int tenp_kop = 1;
    finish = false;
    null_proccess.id = PROG_KOP;
    null_proccess.quantum = 0;
    null_proccess.level = PRIORITY_LEVELS-1;
    null_proccess.state = STATE_UNDEFINED;
    null_thread.tid = 0;
    memory.Text_partition_start = WORD_QUANT;
    next_Text = memory.Text_partition_start;
    memory.Data_partition_start = (WORD_QUANT/3)*2;
    next_Data = memory.Data_partition_start;
    memory.kernel_partition_start = WORD_QUANT/3;
    next_pageT = memory.kernel_partition_start;
    next_p_id = 0;
    for (i=0; i<cpus.cpu_quant; i++)
    for (j=0; j<cpus.core_quant; j++)
    for (k=0; k<cpus.thread_quant; k++) {
        cpus.cct[i][j][k].tid = i*cpus.core_quant*cpus.thread_quant+j*cpus.thread_quant+k+1;
        pthread_mutex_init(&cpus.cct[i][j][k].mutex_e, NULL);
        cpus.cct[i][j][k].executing = null_proccess;
        cpus.cct[i][j][k].free = true;
    }
    for (i=0; i<PRIORITY_LEVELS; i++) {
        first_p[i] = 0;
        last_p[i] = 0;
        for (j=0; j<PROC_KOP_MAX; j++)
            proccess_queue[i][j] = null_proccess;
    }
    
    if (pthread_create(&prozesu_kargatzailea_tid, NULL, prozesu_kargatzailea, NULL) != 0) {
        printf("%sErrorea prozesu kargatzailea martxan jartzean.\n", KNRM);
        exit(4);
    }
    printf("%sProzesu kargatzailea martxan jarri da.\n", KNRM);

    if (pthread_create(&scheduler_dispatcher_tid, NULL, scheduler_dispatcher, NULL) != 0) {
        printf("%sErrorea scheduler-a martxan jartzean.\n", KNRM);
        exit(4);
    }
    printf("%sScheduler-a martxan jarri da.\n", KNRM);

    if (pthread_create(&erlojua_tid, NULL, erlojua, &tenp_kop) != 0) {
        printf("%sErrorea erlojua martxan jartzean.\n", KNRM);
        exit(4);
    }
    printf("%sErlojua martxan jarri da.\n", KNRM);

    if (pthread_create(&tenporizadorea_tid, NULL, tenporizadorea, NULL) != 0) {
        printf("%sErrorea tenporizadorea martxan jartzean.\n", KNRM);
        exit(4);
    }
    printf("%sTenporizadorea martxan jarri da.\n", KNRM);

    /*if (pthread_create(&prozesu_exekutatzailea_tid, NULL, prozesu_exekutatzailea, NULL) != 0) {
        printf("%sErrorea prozesu exekutadoreak martxan jartzean.\n", KNRM);
        exit(4);
    }
    printf("%sProzesu exekutatzailea martxan jarri da.\n", KNRM);*/

    while (!finish) {
        printf("%sPrograma exekutatzen bukatzeko F karakterea idatzi.\n", KNRM);
        scanf("%c", &fin);
        if (fin == 'f' || fin == 'F')
            finish = true;
    }

    usleep(2*1e6);

    printf("%sPrograma itzaltzen...\n", KNRM);
    for (i=0; i<tenp_kop; i++)
        pthread_cond_signal(&cond);
    usleep(2*1e6);
    pthread_cond_signal(&cond2);
    //pthread_cond_signal(&cond_ep);
    pthread_cond_signal(&cond_pk);
    pthread_cond_signal(&cond_sd);

    printf("%sErlojua itzaltzen...\n", KNRM);
    pthread_join(erlojua_tid, NULL);
    printf("%sTenporizadorea itzaltzen...\n", KNRM);
    pthread_join(tenporizadorea_tid, NULL);
    printf("%sProzesu kargatzailea itzaltzen...\n", KNRM);
    pthread_join(prozesu_kargatzailea_tid, NULL);
    printf("%sScheduler-dispatcher-a itzaltzen...\n", KNRM);
    pthread_join(scheduler_dispatcher_tid, NULL);
    //printf("%sProzesu exekutatzailea itzaltzen...\n", KNRM);
    //pthread_join(prozesu_exekutatzailea_tid, NULL);

    printf("%sMutex eta baldintzak ezabatzen...\n", KNRM);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutex_pk);
    pthread_mutex_destroy(&mutex_sd);
    pthread_mutex_destroy(&mutex_ep);
    pthread_mutex_destroy(&mutex_proccess_queue);
    for (i=0; i<cpus.cpu_quant; i++)
    for (j=0; j<cpus.core_quant; j++)
    for (k=0; k<cpus.thread_quant; k++) {
        pthread_mutex_destroy(&cpus.cct[i][j][k].mutex_e);
    }
    pthread_cond_destroy(&cond);
    pthread_cond_destroy(&cond2);
    pthread_cond_destroy(&cond_pk);
    pthread_cond_destroy(&cond_sd);
    pthread_cond_destroy(&cond_ep);
    return 0;
}