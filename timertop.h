#include "list.h"


#define VERSION "Timer Top v 0.9.8"
#define HEADER	"PID           Command   Freq(Hz)   Count           Function       Address "
#define HEADER_FILE	"No.   PID         Command      Count         Function       Address \n"
#define BLANK	"                                                                            "
//#define FUNCTIONS	"Ticks:        Period: 1 s (Up/Dn) Skip Low Hz(z): Y Clear(c) Start/Stop(s)"
  #define FUNCTIONS	"Ticks:           Period: 1 s (Up/Dn)      Skip Low Hz(z): Y      Clear(c) "


typedef struct sys_map_list {
        int count;
	long unsigned int func_address;
	char func_name[23];
} sys_map;


typedef struct proc_timer_info {
	unsigned int count;
	long unsigned int func_address;
	long unsigned int func_count;
	long unsigned int pid;
	char * cmdline;
	int 	diff_count;	/* Store the difference couting between 2 samples */
	char func_name[23];
} proc_timer;


/* Prototypes */
int get_ticks(void);
void read_proc_top_info(void);
void read_system_map_info(void);
//void proc_timer_list_print(const List *proc_timer_list);
void start_background_acquisition(int interval);
void destroy_list_proc_timer(List *list);
void destroy_list_sys_map(List *list);
void print_help();
	
