/*
 * This is Timertop in C
 *
 * Daniel Petrini - d.pensator@gmail.com
 * 
 * Instituto Nokia de Tecnologia - INdT - Manaus
 * November 2005
 *
 * Build: make 
 * Dependencies: lib ncurses
 * Usage: "timertop" for interactive mode
 * 	  "timertop -t 10" for background (less intrusive) acquisition mode
 * 	  "timertop -h" for help and aditional info
 * 
 * TODO:
 * 	Improve build system (autoconf)
 * 	Include Tony's pmstats information about batteries
 *	Include stop mode for last screen in interactive mode (w/out low Hz)
 *	Include more info about PM ?
 * 	
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>                  
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>

#include "list.h"
#include "timertop.h"

#define DATE	"\nDate release: 2005-12-16\n"

#define DEBUG	0

List		sys_map_list;
list_element	*sys_map_element;

List		proc_timer_list;
list_element	*proc_timer_element;

WINDOW * mainwin;
	
int main(int argc, char * argv[]) {

	proc_timer * proc_timer_p;
	sys_map	* sys_map_p;

	int i, j, i_max, ch;
	int exit_loop = 0;
	char buf[80];

	unsigned int period = 1;	/* The sleep time */

	int diff_flag = 1;		/* Flag to compare with different timer countings */
	int start = 1;			/* Toggle state of proc - start/stop */
	int bg_mode = 0;
		
	int found = 0;			/* u */

	FILE * output = NULL;	
	
	char out_func_name[23];
	
	/* Process options */
	if (argc > 1) {
		int interval;
		
		if (!strcmp(argv[1], "-t")) {
			printf("Background acquisition mode\n");
			if (argc < 3) {
				printf("User must supply a period in seconds\n");
				exit(EXIT_FAILURE);
			}
				
			interval = atoi(argv[2]);
			if (interval < 3601) {
				char filename[30];
				char str[40];
				struct timeval tv;
				struct tm* ptm;
		
				start_background_acquisition(interval);
				
				/* Prepare the file name */
				gettimeofday(&tv, NULL);	
				ptm = localtime(&tv.tv_sec);
				strftime(str, sizeof(str), "%Y%m%d.%H%M%S", ptm);
				sprintf(filename, "timer_info.%s", str);
				printf("file: %s\n", filename);
					
				output = fopen(filename, "w");
				if (!output)
					exit(EXIT_FAILURE);
				
				fprintf(output,
					VERSION
					"\n"
					HEADER_FILE
				);
				
				diff_flag = 0;
				bg_mode = 1;
								
			} else {
				printf("Invalid Interval... exiting\n");
				return 0;
			}
		} else if (!strcmp(argv[1], "-h")) {
			print_help();
			exit(0);
		}
		
	}

	if (bg_mode == 0 ) {	
		/*  Initialize ncurses  */
		if ( (mainwin = initscr()) == NULL ) {
			fprintf(stderr, "Error initialising ncurses.\n");
			exit(EXIT_FAILURE);
		}

		cbreak();			/* No input buffering */
		//noecho();			/* Dont print char in screen */
		nodelay(stdscr, TRUE);		/* Non blocking getch */
		keypad(stdscr, TRUE);		/* Enable chars like UP/Down */
		
		mvaddstr(0, 1, VERSION);
		mvaddstr(1, 1, FUNCTIONS);
		attron(A_REVERSE);
		mvaddstr(2, 1, HEADER);
		attroff(A_REVERSE);

		refresh();
	}
  
	/**  Initialize the linked list */
	list_init(&sys_map_list);
	list_init(&proc_timer_list);
	
	/* Reads system.map file in a list */
    	read_system_map_info();

	i_max = 2;

	do {
	
	/* Read the proc file and compares with list ans display on the screen*/
	read_proc_top_info();    

	i = 2;
	
	/* Now iterate through the proc_timer list to show something useful */
	proc_timer_element = list_head(&proc_timer_list);

	for (; ; ) {
		found = 0;
	
		if (!list_size(&proc_timer_list))
			break;
		
		proc_timer_p = list_data(proc_timer_element);
	
		/* If we don't have the function name grab it from system.map list */
		if (!(proc_timer_p->func_name[0]) ) {

			sys_map_element = list_head(&sys_map_list);
			for(;;) {
				sys_map_p = list_data(sys_map_element);

				/* Comparison by address */
				if (sys_map_p->func_address == proc_timer_p->func_address) {

					strncpy(proc_timer_p->func_name, sys_map_p->func_name, 22);
					proc_timer_p->func_name[22] = '\0';
					found = 1;
					
					/* Found, so ... */
					break;
				}
				sys_map_element = list_next(sys_map_element);		
				if (!sys_map_element)
					break;
			}
			/* Not found in System.map: module function */
			if (found == 0) {
				strcpy(proc_timer_p->func_name, "(module)");
			}

		
		}
		
		/* Prevent from showing lines with undesireable low frequencies 
		 * showing only the lines that contains modifications since the 
		 * last reading  and to avoid huge listing of process like 
		 * "gnome-cups-icon" appearing dozens of times 
		 * (can be switched off with 's' key)*/	
		if (proc_timer_p->diff_count > diff_flag - 1) {
		
			i++;		
			if (bg_mode == 0) {	/* ncurses interative */

				mvaddstr(i, 0, BLANK);
				
				sprintf(buf, "%5ld", proc_timer_p->pid);
				mvaddstr(i, 0, buf);
				sprintf(buf, "%17s ", proc_timer_p->cmdline);
				mvaddstr(i, 6, buf);
				sprintf(buf, "%5d", proc_timer_p->diff_count/period);
				mvaddstr(i, 27, buf);
				sprintf(buf, "%8ld", proc_timer_p->func_count);
				mvaddstr(i, 33, buf);
				
				snprintf(out_func_name, 20, "%s", proc_timer_p->func_name);
				mvaddstr(i, 44, out_func_name);

				
				sprintf(buf ," %lx", proc_timer_p->func_address);
				mvaddstr(i, 66, buf);

			} else {		/* background file saving */
				if (i != 3) 	/* skip line = "Disabled" in the proc file */
				  fprintf(output, " %2d %5ld %17s %8ld %22s %lx\n", i - 3, proc_timer_p->pid, proc_timer_p->cmdline,
									    proc_timer_p->func_count,  proc_timer_p->func_name,
									    proc_timer_p->func_address);
				exit_loop = 1;	/* Just once, acquisition already made properly */
			}
		}
		
		if (i > i_max)
			i_max = i;

		
   		proc_timer_element = list_next(proc_timer_element);
		if (!proc_timer_element)
		       break;	
	}

	if (exit_loop == 1) {
		system("echo -n \"stop\" > /proc/timer_input");
		break;
	}
	
	/* Some user interaction */	
	ch = getch();
	switch (ch) {
		case 'q':
			system("echo -n \"stop\" > /proc/timer_input");
			exit_loop = 1;
		break;
		case 'c':
			system("echo -n \"clear\" > /proc/timer_input");
			destroy_list_proc_timer(&proc_timer_list);
			list_init(&proc_timer_list);
		break;
		case 's': /* start/stop ... not in use right now... 20051124 */
			if (start == 1) {
				system("echo -n \"stop\" > /proc/timer_input");
				start = 0;
			} else {
				system("echo -n \"start\" > /proc/timer_input");
				start = 1;
			}
		break;
		case 'z':
			if (diff_flag == 1) {
				diff_flag = 0;
				sprintf(buf, "N");
				mvaddstr(1, 59, buf);
			} else { 
				diff_flag = 1;
				sprintf(buf, "Y");
				mvaddstr(1, 59, buf);
			}
		break;
		case KEY_UP:
			period++;
			sprintf(buf, "%2d", period);
			mvaddstr(1, 25, buf);
		break;
		case KEY_DOWN:
			period--;
			if (period < 1)
				period = 1;
			sprintf(buf, "%2d", period);
			mvaddstr(1, 25, buf);
		break;
	}	

	/* Erase unused lines */
	for(j = i+1; j <= i_max; j++)
		mvaddstr(j, 0, BLANK);

	/* Update ticks value */
	sprintf(buf, "%4d", get_ticks()/period);
	mvaddstr(1, 8, buf);

	/* Put cursor in a good place */	
	wmove(stdscr, 1, 74);
	
	/* Call refresh to show our changes */
	refresh();
	
	/* wait some time to user doesnot get in a hurry */
	sleep(period);

	} while(1);
	
	/*  Clean up after ourselves  */
	if (bg_mode == 0) {
		delwin(mainwin);
		endwin();
		refresh();
	} else {
		fclose(output);
	}

	destroy_list_proc_timer(&proc_timer_list);
	destroy_list_sys_map(&sys_map_list);

	return EXIT_SUCCESS;
}

/*
 * Handy function to exit ncurses
 */
void ncurses_exit(void)
{
	delwin(mainwin);
	endwin();
	refresh();
}

/* 
 * Send proper strings to timer_input proc entry
 */
void start_background_acquisition(int interval)
{

	char str[40];
	
	printf("With interval = %d\n", interval);
	printf("Starting...");
	printf("echo -n \"stop\" > /proc/timer_input\n");
	system("echo -n \"stop\" > /proc/timer_input");
	printf("echo -n \"clear\" > /proc/timer_input\n");
	system("echo -n \"clear\" > /proc/timer_input");
	printf("echo -n \"start\" > /proc/timer_input\n");
	system("echo -n \"start\" > /proc/timer_input");
	sprintf(str, "sleep %d", interval);
	printf("%s\n", str);
	sleep(interval);
	
	printf("echo -n \"stop\" > /proc/timer_input\n");
	system("echo -n \"stop\" > /proc/timer_input");

}

/*
 * Read /proc/timer_info and sum-up it by function address and pid
 */
void read_proc_top_info()
{
	FILE *fp=NULL;
	char *aux;

	char * delimiter = " \t\n";

	char *token;

	char buf[80];

	int j, i, k;
	int exist_in_list = 0;
	int not_ascii = 0;
	int tok_size;
	
	proc_timer * proc_timer_p;

	int count_aux = 0;
	long unsigned int func_count_aux = 0;
	long unsigned int pid_aux = 0;
	char * cmdline_aux = NULL;	
	long unsigned int func_address_aux = 0;

	static int first_time = 1;	
	i = 0;
	
	/* Activate proc output */
	if (first_time == 1) {
		system("echo start > /proc/timer_input");
		first_time = 0;
	}
	
	if ((fp=fopen("/proc/timer_info", "r"))==NULL) {
		ncurses_exit();
		printf(
			"Error reading proc entry\n"
			"Maybe this kernel does not have Timer info support?\n"
		);
		exit(EXIT_FAILURE);
	}
	
	while ((fgets(buf, 78, fp))!=NULL) {

		/* Avoid first line of file */	
		if (i++ == 0) 
			continue;
		
		//if (i == 2)
		//	if (!strncmp("Disabled", buf, 8)) {
		//		system("echo -n \"start\" > /proc/timer_input");
		//		ncurses_exit();
		//		printf("Kernel Timertop Disabled\n");
		//		printf("echo -n \"start\" > /proc/timer_input \nto activate\n");
		//		exit(EXIT_FAILURE);
		//	}
				
		
		/* Parse from the file */
		aux = buf;
		j = 0;
		do {
			token = strsep (&aux, delimiter);	/* separate by the delimiter (change it for \0 in aux) */
			if (*token == '\0') {
				break;
				
			}
			switch (j++) {
				case 0:
					count_aux = i;
					func_address_aux = strtoul(token, NULL, 16);
				break;
				case 1:
					func_count_aux = atol(token);
				break;
				case 2:
					pid_aux = atol(token);
					if (pid_aux != 0)	/* There will be a cmdline */
						break;
					if ((cmdline_aux = (char *)realloc(cmdline_aux, strlen("-") + 1)) == NULL ) {
					       printf("Error: realloc 2\n");	
					       exit(EXIT_FAILURE);
					}
					strcpy(cmdline_aux, "-");
				break;
				case 3: /* cmdline */
					tok_size = strlen(token);
					
					/* Sometimes some invalid names comes from proc */
					for (k=0 ; k < tok_size; k++)
						if (!isascii(token[k])) {
							not_ascii = 1;
						}
					
					if (not_ascii == 0) {
						if ((cmdline_aux = (char *)realloc(cmdline_aux, tok_size + 1)) == NULL ) {
						       printf("Error: realloc 2\n");	
						       exit(EXIT_FAILURE);
						}
						strcpy(cmdline_aux, token);
					} else {
						if ((cmdline_aux = (char *)realloc(cmdline_aux, strlen("-") + 1)) == NULL ) {
						       printf("Error: realloc 2\n");	
						       exit(EXIT_FAILURE);
						}
						strcpy(cmdline_aux, "--"); /* Invalid name */
						not_ascii = 0;
					}
							
				break;
				default:
					//printf("Error: switch proc [%d], %d, %s\n",i, j, token);
				break;
				
			}
			
		} while (aux);

		/* 
		 * We can not append identical entries in the list, so that we need to make
		 * a test to verify if the current line from /proc/timer_info is about a
		 * already entered (functions address, cmdline) or not 
		 *
		 * If the entry exists update the counting only
		 */
		proc_timer_element = list_head(&proc_timer_list);
	
		for (; ;) {
			if (!list_size(&proc_timer_list))
				break;
			proc_timer_p = list_data(proc_timer_element);
		
			if (!proc_timer_p)
				break;
			
			/* It is only fair to compute by pid */
			if  ( (proc_timer_p->func_address == func_address_aux) && 
						(proc_timer_p->pid == pid_aux) ) {
					//(!strncmp(proc_timer_p->cmdline, cmdline_aux, strlen(cmdline_aux))) ) {
				exist_in_list = 1;
				
				/* Just update the counting */
				proc_timer_p->diff_count = func_count_aux - proc_timer_p->func_count;
				proc_timer_p->func_count = func_count_aux;
			}
		
			proc_timer_element = list_next(proc_timer_element);
			if (!proc_timer_element)
			       break;	
		}


		/* To accomodate exception to above rule
		 * None Hopefully */
		if (exist_in_list) {
			exist_in_list = 0;
		
			continue; /* exist_in_list */
		} else {

			/* IF NOT EXIST IN LIST  */
			if ((proc_timer_p = (proc_timer *)malloc(sizeof(*proc_timer_p))) == NULL) {
				printf("Error: No memory");
				exit(EXIT_FAILURE);
			}
			proc_timer_p->count = count_aux;
			proc_timer_p->func_address = func_address_aux;
			
			proc_timer_p->func_count = func_count_aux;
			proc_timer_p->pid = pid_aux;

			strcpy(proc_timer_p->func_name, "");
			
			if (cmdline_aux) {
				if ((proc_timer_p->cmdline = (char *)malloc(strlen(cmdline_aux) + 1)) == NULL) {
					printf("Error: No memory - 4");
					exit(EXIT_FAILURE);
				}
				strcpy(proc_timer_p->cmdline, cmdline_aux);
			} else {
				if ((proc_timer_p->cmdline = (char *)malloc(strlen("-") + 1)) == NULL) {
					printf("Error: No memory - 5");
					exit(EXIT_FAILURE);
				}
				strcpy(proc_timer_p->cmdline, "-");
			}
			
			/* Insert at last */	
			if (list_insert_next(&proc_timer_list, NULL, proc_timer_p) != 0) {
				printf ("Error inserting list\n");
				return ;
			}	
			
		}	
	}
			
	//proc_timer_list_print(&proc_timer_list);
	
	/* clean the mess */
	if (cmdline_aux != NULL) 
		free(cmdline_aux);
	cmdline_aux = NULL;		
	fclose(fp);
}

#if DEBUG
static void proc_timer_list_print(const List *proc_timer_list) {

	list_element	*element;
	int		i;
	proc_timer	*teste_p;

	printf("List size is %d\n", list_size(proc_timer_list));

	i = 0;
	element = list_head(proc_timer_list);

	while (1) {

		teste_p = list_data(element);
		printf("Pointer: %p proc_timer_list[%03d]=%lx %ld %ld %s\n", teste_p, i,
			       		teste_p->func_address, teste_p->func_count,
				       	teste_p->pid, teste_p->cmdline);

		i++;

		if (list_is_tail(element))
		break;
		else
		element = list_next(element);

	}

	return;

}


char * remove_lf(char * str)
{
	char * orig = str;
	
	for (/*str*/; *str ; str++) {
		if (*str == '\n') {
			*str = '\0';
			break;
		}
	}
	return orig;
}


static void sys_map_list_print(const List *sys_map_list) {

	list_element	*element;
	int		i;
	sys_map		*teste_p;

	printf("List size is %d\n", list_size(sys_map_list));

	i = 0;
	element = list_head(sys_map_list);

	while (1) {

		teste_p = list_data(element);
		fprintf(stdout, "Pointer: %p sys_map_list[%03d]=%lx %s\n", teste_p,
		       			i, teste_p->func_address, teste_p->func_name);

		i++;

		if (list_is_tail(element))
		break;
		else
		element = list_next(element);

	}

	return;

}
#endif

sys_map *n = NULL;

/*
 * This Function read the System.map file that should be in this same directory.
 * And stores its content in a linked list for further processing.
 */
void read_system_map_info()
{
	FILE *fp=NULL;
	char  buf[100];
	char * aux, * token;
	//char * function_aux = NULL;
	char function_aux[22];

	long unsigned int func_address_aux = 0;
	
	int i = 1;
	int j;
	
	sys_map *p = NULL;
	
	
	/*  Perform some linked list operations */
	sys_map_element = list_head(&sys_map_list);

	if ((fp=fopen("System.map", "r"))==NULL) {
		ncurses_exit();
		printf("System.map of the running kernel required in this directory\n");
		exit(EXIT_FAILURE);
	}
	
	while ((fgets(buf, 100, fp))!=NULL) {

		aux = buf;
		j = 0;
		do {
			token = strsep (&aux, " \n\t");	/* separate by the delimiter (change it for \0 in aux) */
			if (*token == '\0') {
				break;
				
			}
			switch (j++) {
				case 0:
					func_address_aux = strtoul(token, NULL, 16);
				break;
				case 1:
					//dont care
				break;
				case 2:
					strncpy(function_aux, token, 22);
					function_aux[22] = '\0';
				break;
				default:
					//printf("Error: switch sys, %d, %s\n", j, token);
				break;
				
			}
			
		} while (aux);

		/* feed the list */
		p = (sys_map *)malloc(sizeof(*p));

		if (!p)
			exit (EXIT_FAILURE);
		
		p->count = i++;
		p->func_address = func_address_aux;
		
		strncpy(p->func_name, function_aux, 23);

		if (list_insert_next(&sys_map_list, NULL, p) != 0) {
			printf ("Error\n");
			return ;
		}	

	}

	//sys_map_list_print(&sys_map_list);
	
	fclose(fp);
	
}

/* Thanks to Tony Lindgren for ticks calculation, from pmstats-0.2 */
int get_ticks(void)
{

	FILE * fp;

	char buf [80];

	char * aux;
	char * token;

	long int tick;
	long int diff_ticks = 0;
	static long int last_tick;

	int omap = 0; /* will be a cmdline parameter */

	if ((fp=fopen("/proc/interrupts", "r"))==NULL) {
		printf("Error reading proc entry\n");
		exit(EXIT_FAILURE);
	}
	
	while ((fgets(buf, 78, fp))!=NULL) {

		if (omap == 1)
			token = strstr(buf, "32KHz");	/* Try omap 32KHz timer first */
		else
			token = strstr(buf, "timer\n");
		
		if (token != NULL) {

			/* Damn hack to get it. Hope it works in other archs... */
			aux = &buf[5];
			aux[15] = '\0';
			tick = atol(aux);
			diff_ticks = tick - last_tick;
			last_tick = tick;
			break;
		}
	}

	/* Put a limit */
	if (diff_ticks > 9999)
		diff_ticks = 9999;
	
	fclose(fp);

	return diff_ticks;
}

void destroy_list_proc_timer(List *list) {

	void	*data;
	proc_timer * tmp;
	
	while (list_size(list) > 0) {

		if (list_remove_next(list, NULL, (void **)&data) == 0) {

			tmp = (proc_timer *) data;
			free(tmp->cmdline);
			free(data);
	   	}

	}
	memset(list, 0, sizeof(List));
	return;

}

void destroy_list_sys_map(List *list) {

	void	*data;
	
	while (list_size(list) > 0) {

		if (list_remove_next(list, NULL, (void **)&data) == 0) {
			free(data);
	   	}

	}
	memset(list, 0, sizeof(List));
	return;

}

void print_help(void)
{
 	printf(
 		VERSION
 		DATE
 		"\n"
 		"Print statistics about kernel timers\n"
 		"This can be done in an interactive mode, run \"timertop\"\n"
 		"or in background mode, run \"timertop -t 5\" for 5 seconds of data gathering\n"
  	
 		"\n"
 		"In interactive mode you can\n"
  		//"   	start/stop the refreshing with key 's'\n"
 		"   	increase/decrease the sample period with keys 'UP/Down'\n"
 		"   	filter less than 1 Hz timers with key 'z'\n"
 		"   	clear timers in proc entry with key 'c'\n"
 
 		"\n"
 		"The System.map file of the current kernel must be in the same directory of timertop\n"
 		"The current kernel must have TIMER_INFO support enabled\n"
 		"\n"
 		"The program is based on a (required) kernel feature which exports data timers in /proc\n"
 		"and can be particularly useful for dynamic tick"
 		" analysis and timers optimization in the system\n"
 		"Timers are summed up based on process pid and timer function\n"
 		"\n"
 		"Bug report: d.pensator@gmail.com\n"
 	);
}
