#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <math.h>

#ifndef KERNEL_H
#define KERNEL_H

#define PRIORITY_LEVELS 5   //Prioritate maila kopurua
#define PROC_KOP_MAX    100 //Prozesu kopuru maximoa prioritate maila bakoitzeko

#define SCHEDULER_POLITIC_RORO 'R' // RORO: Round Robin
#define SCHEDULER_POLITIC_LDDQ 'L' // LDDQ: Lehentasun Dinamikoko pixkanakako Degradazioa Quantum inkrementatuz

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
    uint64_t id;
    uint32_t execution_time_needed, time_executed, quantum, level;
    char state;
} PCB;

typedef struct {
    pthread_t tid;
    pthread_mutex_t mutex_e;
    PCB executing;
    uint32_t exec_time;
    bool free;
} thread;

typedef struct {
    thread cct[MAX_CPU_QUANT][MAX_CORE_QUANT][MAX_THREAD_QUANT]; //cct = CPU core thread
    uint8_t cpu_quant, core_quant, thread_quant;
} CPU;

extern pthread_mutex_t mutex, mutex_ps, mutex_sd,
    mutex_ep, mutex_proccess_queue;
extern pthread_cond_t cond, cond2, cond_ps, cond_sd, cond_ep;
extern PCB proccess_queue[PRIORITY_LEVELS][PROC_KOP_MAX], null_proccess;
extern CPU cpus;
extern thread null_thread;
extern uint64_t next_p_id, erlojua_tid, tenporizadorea_tid, prozesu_sortzailea_tid, scheduler_dispatcher_tid, prozesu_exekutatzailea_tid;
extern uint32_t first_p[PRIORITY_LEVELS], last_p[PRIORITY_LEVELS], done, timer_ps, timer_sd, frequence;
extern char scheduler_politic;
extern bool finish;

extern void *erlojua(void *tenp_kop);
extern void *tenporizadorea();
extern void *prozesu_sortzailea();
extern void *scheduler_dispatcher();
extern void *prozesu_exekutatzailea();
extern void next_free_ocup_cct(int cct[3], bool free);

#endif