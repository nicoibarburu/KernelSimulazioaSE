#include "kernel.h"

void copy_to_memory(word memory, uint32_t from) {
    memory.word[0] = (from >> 24) & 0xFF;
    memory.word[1] = (from >> 16) & 0xFF;
    memory.word[2] = (from >> 8) & 0xFF;
    memory.word[3] = from & 0xFF;
}

void *prozesu_kargatzailea() {
    FILE *file;
    uint8_t line[20];
    uint32_t text_count, data_count, word;
    int level;
    char *prog_name = "./Programak/progXXX.elf";
    pthread_mutex_lock(&mutex_pk);
    while(1) {
        pthread_cond_wait(&cond_pk, &mutex_pk);
        if (finish)
            break;
        // Prozesuaren maila aukeratu
        if (scheduler_politic == SCHEDULER_POLITIC_RORO)
            level = 0;
        else if (scheduler_politic == SCHEDULER_POLITIC_LDDQ)
            level = (rand()%PRIORITY_LEVELS);
        pthread_mutex_lock(&mutex_proccess_queue);
        // Prozesu ilara ez badago beteta prozesu berri bat sartu
        if (last_p[level] != first_p[level] || proccess_queue[level][first_p[level]].state == STATE_UNDEFINED) {
            PCB proccess;
            // Irakurri behar dugun fitxategiaren izena definitu
            if (next_p_id < 10)
                sprintf(prog_name, "%s%ld%s", PROG_FILE_NAME3, next_p_id, PROG_FILE_NAME_EXT);
            else if (next_p_id < 100)
                sprintf(prog_name, "%s%ld%s", PROG_FILE_NAME2, next_p_id, PROG_FILE_NAME_EXT);
            else
                sprintf(prog_name, "%s%ld%s", PROG_FILE_NAME1, next_p_id, PROG_FILE_NAME_EXT);
            // Fitxategia irakurri
            if((file = fopen(prog_name, "r")) == 0) {
                printf("%sErrorea %s fitxategia irekitzerakoan.\n", KCYN, prog_name);
                continue;
            }
            text_count = 0;
            data_count = 0;
            // Fitxategia lerroz lerro aztertu eta memorian kopiatu
            while (fgets(line, sizeof(line), file)) {
                line[strcspn(line, "\n")] = '\0';
                // Text atala non hasten den irakurri
                if (strncmp(line, ".text", 5) == 0)
                    proccess.code = (uint32_t)strtoul(line+6, NULL, 16);
                // Data atala non hasten den irakurri
                else if (strncmp(line, ".data", 5))
                    proccess.data = ((uint32_t)strtoul(line+6, NULL, 16))/4;
                // Informazioa memorian kopiatu
                else {
                    word = (uint32_t)strtoul(line, NULL, 16);
                    // Text ataleko informazioa
                    if (text_count < proccess.data) {
                        // Lehenengo datua irakurri behar badugu orri-taula sortu eta lau helbideak sartu behar ditugu
                        // Text atalaren helbide birtual eta fisikoa, data atalaren helbide birtual eta fisikoa
                        if (text_count == 0) {
                            proccess.pgb = next_pageT;
                            copy_to_memory(memory.memory[next_pageT], proccess.code);
                            if (next_pageT-1 > memory.kernel_partition_start)
                                next_pageT = memory.kernel_partition_start;
                            else
                                next_pageT--;
                            copy_to_memory(memory.memory[next_pageT], next_Text);
                            if (next_pageT-1 > memory.kernel_partition_start)
                                next_pageT = memory.kernel_partition_start;
                            else
                                next_pageT--;
                            copy_to_memory(memory.memory[next_pageT], proccess.data);
                            if (next_pageT-1 > memory.kernel_partition_start)
                                next_pageT = memory.kernel_partition_start;
                            else
                                next_pageT--;
                            copy_to_memory(memory.memory[next_pageT], next_Data);
                            if (next_pageT-1 > memory.kernel_partition_start)
                                next_pageT = memory.kernel_partition_start;
                            else
                                next_pageT--;
                        } // if (text_count == 0) bukaera
                        // Irakurritako lerroa memorian gorde text partizioan
                        copy_to_memory(memory.memory[next_Text], word);
                        if (next_Text-1 == memory.Data_partition_start)
                            next_Text = memory.Text_partition_start;
                        else
                            next_Text--;
                        text_count++;
                    } // if (text_count < proccess.data) bukaera
                    else {
                        // Irakurritako lerroa memorian gorde data partizioan
                        copy_to_memory(memory.memory[next_Data], word);
                        if (next_Data-1 == memory.kernel_partition_start)
                            next_Data = memory.Data_partition_start;
                        else
                            next_Data--;
                        data_count++;
                    }
                } // .text eta .data lerroak ez diren kasuaren bukaera
            } // while irakurri fitxategiaren lerroak bukaera
            // Prozesuaren code eta data lerroak gorde
            proccess.code_lines = text_count;
            proccess.data_lines = data_count;
            proccess.id = next_p_id;
            if (scheduler_politic == SCHEDULER_POLITIC_RORO)
                proccess.quantum = (rand()%(int)pow(2, PRIORITY_LEVELS-1))+1;
            else
                proccess.quantum = pow(2, level);
            proccess.level = level;
            proccess.state = STATE_READY;
            next_p_id = (next_p_id+1)%PROG_KOP;
            proccess_queue[level][last_p[level]] = proccess;
            printf("%s%lu prozesua sortuta.\n", KCYN, proccess_queue[level][last_p[level]].id);
            last_p[level] = (last_p[level]+1)%PROC_KOP_MAX;
        }
        else
            printf("%sProzesu kopuru maximoa prozesuaren mailan. Ezin da prozesurik sortu.\n", KCYN);
        pthread_mutex_unlock(&mutex_proccess_queue);
    } // while (1) bukaera
    return 0;
}