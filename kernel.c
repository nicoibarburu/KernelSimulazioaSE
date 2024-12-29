#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define PRIORITY_LEVELS 5   //Prioritate maila kopurua
#define PROC_KOP_MAX    100 //Prozesu kopuru maximoa prioritate maila bakoitzeko

#define SCHEDULER_POLITIC_RORO "RORO" // FCFS: First Come First Served
#define SCHEDULER_POLITIC_DPDQ "DPDQ" // DPDQ: lehentasun Dinamikoko Pixkanakako Degradazioa Quantum inkrementatuz

#define STATE_EXECUTION 'E' //E: Exekuzioan
#define STATE_READY     'R' //R: Prest
#define STATE_BLOCKED   'B' //B: Blokeatuta
#define STATE_UNDEFINED 'U' //U: Definitu gabekoa
#define STATE_FINISHED  'F' //F: Amaituta

#define MAX_CPU_QUANT    4  //CPU kopuru maximoa
#define MAX_CORE_QUANT   8  //core kopuru maximoa cpu-ko
#define MAX_THREAD_QUANT 16 //hari kopuru maximoa core-ko

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

typedef struct {
    unsigned long id;
    unsigned int execution_time_needed, time_executed, quantum, level;
    char state;
} PCB;

typedef struct {
    pthread_t tid;
    bool free;
} thread;

typedef struct {
    thread cct[MAX_CPU_QUANT][MAX_CORE_QUANT][MAX_THREAD_QUANT];
    unsigned short cpu_quant, core_quant, thread_quant;
} CPU;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER, mutex_ps = PTHREAD_MUTEX_INITIALIZER, mutex_sd = PTHREAD_MUTEX_INITIALIZER,
    mutex_ep = PTHREAD_MUTEX_INITIALIZER, mutex_proccess_queue = PTHREAD_MUTEX_INITIALIZER, *mutex_executing;
pthread_cond_t cond, cond2, cond_ps, cond_sd, cond_ep;
PCB proccess_queue[PRIORITY_LEVELS][PROC_KOP_MAX], *executing, null_proccess;
CPU cpus;
unsigned long next_p_id;
unsigned int first_p[PRIORITY_LEVELS], last_p[PRIORITY_LEVELS], done, timer_ps, timer_sd, frequence;
char *scheduler_politic;

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

void *tenporizadorea() {
    long unsigned int tick_kont = 0, seg_kont = 0;
    pthread_mutex_lock(&mutex);
    while(1) {
        done++;
        tick_kont++;
        printf("\n%s%lu tick\n", KGRN, tick_kont);
        if (tick_kont%frequence == 0) {
            seg_kont++;
            printf("%s%lu segundu pasa dira.\n", KGRN, seg_kont);
            pthread_cond_broadcast(&cond_ep);
            if (seg_kont%timer_ps == 0)
                pthread_cond_signal(&cond_ps);
            if (seg_kont%timer_sd == 0)
                pthread_cond_signal(&cond_sd);
        }
        pthread_cond_signal(&cond);
        pthread_cond_wait(&cond2, &mutex);
    }
}

void *prozesu_sortzailea() {
    int level;
    pthread_mutex_lock(&mutex_ps);
    while(1) {
        pthread_cond_wait(&cond_ps, &mutex_ps);
        if (scheduler_politic == SCHEDULER_POLITIC_RORO)
            level = 0;
        else
            level = (rand()%PRIORITY_LEVELS);
        if (last_p[level] != first_p[level] || proccess_queue[level][first_p[level]].state == STATE_UNDEFINED) {
            PCB proccess;
            proccess.id = next_p_id;
            proccess.execution_time_needed = (rand()%(int)pow(2, PRIORITY_LEVELS-1))+1;
            proccess.time_executed = 0;
            if (scheduler_politic == SCHEDULER_POLITIC_RORO)
                proccess.quantum = (rand()%proccess.execution_time_needed)+1;
            else {
                proccess.quantum = pow(2, level);
            }
            proccess.level = level;
            proccess.state = STATE_READY;
            next_p_id++;
            pthread_mutex_lock(&mutex_proccess_queue);
            if (last_p[level] != first_p[level] || proccess_queue[level][first_p[level]].state == STATE_UNDEFINED) {
                printf("%s%lu prozesua sortuta.\n", KCYN, proccess.id);   
                proccess_queue[level][last_p[level]] = proccess;
                last_p[level] = (last_p[level]+1)%PROC_KOP_MAX;
            }
            else
                printf("%sProzesu kopuru maximoa prozesuaren mailan. Ezin da prozesurik sortu.\n", KCYN);
            pthread_mutex_unlock(&mutex_proccess_queue);
        }
        else
            printf("%sProzesu kopuru maximoa prozesuaren mailan. Ezin da prozesurik sortu.\n", KCYN);
    }
}

void *scheduler_dispatcher() {
    int i, level, out_level, next_t_exec = -1;
    double out_exec_time;
    pthread_mutex_lock(&mutex_sd);
    while(1) {
        pthread_cond_wait(&cond_sd, &mutex_sd);
        printf("%sSecheduler-a aktibatuta.\n", KYEL);
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
        else {
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

void *execute_proccess(void *tidv) {
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

int main(int argc, char *argv[]) {
    if (argc != 8) {
        printf("%sErabilera: %s erlojuaren_maiztasuna(Hz) zenbat_segunduro_prozesua_sortu zenbat_segunduro_schedulerra_aktibatu CPU_kant CPU-ko_core_kant core-ko_hari_kant schedule(RORO edo DPDQ)\n", KNRM, argv[0]);
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
    if (strcmp(argv[7], SCHEDULER_POLITIC_RORO) != 0 && strcmp(argv[7], SCHEDULER_POLITIC_DPDQ) != 0) {
        printf("%sScheduler-a RORO edo DPDQ izan behar da.\n", KNRM);
        exit(2);
    }
    else {
        if (strcmp(argv[7], SCHEDULER_POLITIC_RORO) == 0)
            scheduler_politic = SCHEDULER_POLITIC_RORO;
        else
            scheduler_politic = SCHEDULER_POLITIC_DPDQ;
    }

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
        if (pthread_create(&cpus.cct[nextcct[0]][nextcct[1]][nextcct[2]].tid, NULL, execute_proccess, tid) != 0) {
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