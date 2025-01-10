kernel: kernel.h kernel.c erlojua.c tenporizadorea.c prozesu_kargatzailea.c scheduler_dispatcher.c prozesu_exekutatzailea.c
	gcc kernel.h kernel.c erlojua.c tenporizadorea.c prozesu_kargatzailea.c scheduler_dispatcher.c prozesu_exekutatzailea.c -o kernel -pthread -lm -g

debug: kernel
	gdb ./kernel