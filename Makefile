kernel: kernel.h kernel.c erlojua.c tenporizadorea.c prozesu_sortzailea.c scheduler_dispatcher.c prozesu_exekutatzailea.c
	gcc kernel.h kernel.c erlojua.c tenporizadorea.c prozesu_sortzailea.c scheduler_dispatcher.c prozesu_exekutatzailea.c -o kernel -pthread -lm