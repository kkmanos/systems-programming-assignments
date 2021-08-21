#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>
#include "generic_list/generic_list.h"
#include "globals.h"
#include "dir_traversal/dir_traversal.h"
#include "mystring/mystring.h"
#include "sockets/sockets.h"
#include "travel_monitor/travel_monitor.h"

// used for execute command returning flags
#define PARSE_ERROR -1
#define EXIT_CMD 0
#define PARSE_COMPLETE 1

int country_dirs_cmp(void *v1, void *v2);
int isnumber(char* s);
int get_npaths(list_t *l, int i);

int execute_command(travel_monitor_t * tm, char *cmd, list_t *country_dirs, char *input_dir);


int main(int argc, char** argv) {



	char input_dir[100];
    unsigned int num_monitors;
    char *bloom_size = NULL;
    char *sock_buffer_size = NULL;

	const int max_arg_index = argc - 1;
	if (argc != 13) {
		printf("==ERROR: Wrong number of arguments.\n");
        printf("Usage: \n./travelMonitorServer –m numMonitors -b socketBufferSize -s sizeOfBloom -i input_dir -c cyclicBufferSize -t numThreads\n");
		exit(EXIT_FAILURE);
	}
	for (int i = 1; i <= max_arg_index; i++) { // parsing command line args
		if (!strcmp(argv[i], "-i") && i+1 <= max_arg_index) { // if read "-i" and there is next arg
            strcpy(input_dir, argv[i+1]);
            DIR *d = opendir(input_dir);
            if (d != NULL)
                closedir(d);
            else if (ENOENT == errno) {
                printf("==Error: input directory \"%s\" does not exist\n", input_dir);
                exit(EXIT_FAILURE);
            }
		}

		if (!strcmp(argv[i], "-b") && i+1 <= max_arg_index) { // if read "-b" and there is another arg
			if (isnumber(argv[i+1])) { // if next arg is a number, then store it
                sock_buffer_size = argv[i+1];  // store it as string
				SOCK_BUFFER_SZ = (unsigned int) atoi(argv[i+1]);
			}
			else {
				printf("==ERROR: -b arg not a number\n");
                printf("Usage: \n./travelMonitorServer –m numMonitors -b socketBufferSize -s sizeOfBloom -i input_dir -c cyclicBufferSize -t numThreads\n");
				exit(EXIT_FAILURE);
			}
		}
		if (!strcmp(argv[i], "-s") && i+1 <= max_arg_index) { // if read "-s" and there is another arg
			if (isnumber(argv[i+1])) { // if next arg is a number, then store it
                bloom_size = argv[i+1]; // store it as a string
				BLOOM_SZ = (unsigned int) atoi(argv[i+1]);
			}
			else {
				printf("==ERROR: -s arg not a number\n");
                printf("Usage: \n./travelMonitorServer –m numMonitors -b socketBufferSize -s sizeOfBloom -i input_dir -c cyclicBufferSize -t numThreads\n");
				exit(EXIT_FAILURE);
			}
		}
		if (!strcmp(argv[i], "-m") && i+1 <= max_arg_index) { // if read "-m" and there is another arg
			if (isnumber(argv[i+1])) { // if next arg is a number, then store it
				num_monitors = (unsigned int) atoi(argv[i+1]);
			}
			else {
				printf("==ERROR: -m arg not a number\n");
                printf("Usage: \n./travelMonitorServer –m numMonitors -b socketBufferSize -s sizeOfBloom -i input_dir -c cyclicBufferSize -t numThreads\n");
				exit(EXIT_FAILURE);
			}
		}
		if (!strcmp(argv[i], "-c") && i+1 <= max_arg_index) {
			if (isnumber(argv[i+1])) { // if next arg is a number, then store it
				CYCLIC_BUFFER_SZ = (unsigned int) atoi(argv[i+1]);
			}
			else {
				printf("==ERROR: -c arg not a number\n");
                printf("Usage: \n./travelMonitorServer –m numMonitors -b socketBufferSize -s sizeOfBloom -i input_dir -c cyclicBufferSize -t numThreads\n");
				exit(EXIT_FAILURE);
			}			
		}
		if (!strcmp(argv[i], "-t") && i+1 <= max_arg_index) {
			if (isnumber(argv[i+1])) {
				NUM_THREADS = (unsigned int) atoi(argv[i+1]);
			}
			else {
				printf("==ERROR: -t arg not a number\n");
                printf("Usage: \n./travelMonitorServer –m numMonitors -b socketBufferSize -s sizeOfBloom -i input_dir -c cyclicBufferSize -t numThreads\n");
				exit(EXIT_FAILURE);
			}
		}
	}


    list_t *country_dirs = list_create(country_dirs_cmp, NULL); // {key: dirname, value: pid}
	char **subdirs;
    int n_subdirs = get_subdirs(input_dir, &subdirs);
	int mon_index = 0;
	// insert all country dirs into the country_dirs list with a monitor serial number [0..num_monitors-1]
	for (int k = 0; k < n_subdirs; k++) {
		if (!strcmp(subdirs[k], ".") || !strcmp(subdirs[k], "..")) continue;
		int *pidnum = NULL;
		pidnum =  malloc(sizeof(int));
		*pidnum = mon_index;
		char *newdir = NULL;
		newdir =  malloc(sizeof(char) * (strlen(subdirs[k]) + 1));
		strcpy(newdir, subdirs[k]);
		list_insert(country_dirs, newdir, pidnum);

		mon_index++;
		if (mon_index == num_monitors) mon_index = 0; // reset

	}
    pid_t childpid = 20;
	srand(time(NULL));
	unsigned int base_port = 20001; // (rand() % (65000 - 1024 + 1)) + 1024 ;
    // creating the children
    int i;
    for (i = 0; i < num_monitors; i++) // only parent increments
        if ( (childpid = fork()) <= 0 ) // if its a child, then break
            break;


    if (childpid == 0) {
		unsigned int npaths = get_npaths(country_dirs, i);
		unsigned int nargs = 11 + npaths + 1;
        char **args = (char **) malloc(sizeof(char *) * nargs);
		args[0] = string_create("./monitorServer");
		args[1] = string_create("-p");
		args[2] = malloc(sizeof(char) * 10);
		sprintf(args[2], "%d", base_port + i);

		args[3] = string_create("-t");
		args[4] = malloc(sizeof(char) * 10);
		sprintf(args[4], "%d", NUM_THREADS);

		args[5] = string_create("-b");
		args[6] = malloc(sizeof(char) * 10);
		sprintf(args[6], "%d", SOCK_BUFFER_SZ);

		args[7] = string_create("-c");
		args[8] = malloc(sizeof(char) * 10);
		sprintf(args[8], "%d", CYCLIC_BUFFER_SZ);


		args[9] = string_create("-s");
		args[10] = malloc(sizeof(char) * 10);
		sprintf(args[10], "%d", BLOOM_SZ);
		int argcount = 11;

		for (list_node_t *n = country_dirs->head; n; n = n->next) {
			char *dirpath = (char *) n->key;
			int *mon_num = (int *) n->value;
			if (*mon_num == i) {
				char temp[100] = "";
				strcpy(temp, input_dir);
				strcat(temp, "/");
				strcat(temp, dirpath);
				args[argcount] = string_create(temp);
				argcount++;
			}
		}
		args[argcount] = NULL;
        int ret_ex = execv("./monitorServer", args);
		if (ret_ex == -1) {
			perror("monitorServer");
		}
		for (int k = 0; k < nargs; k++)
			free(args[k]);
		free(args);
		for (int k = 0; k < n_subdirs; k++) 
			free(subdirs[k]);
		free(subdirs);
		exit(0);
    }

	#ifdef DEBUG
	printf("BEFORE CONNECTIONS CREATED\n");
	#endif

	// create connections
	connection_t *con_arr[num_monitors];
	travel_monitor_t *tm = travel_monitor_create(con_arr, num_monitors);
	for (int k = 0; k < num_monitors; k++) {
		con_arr[k] = connection_create(base_port + k, "127.0.0.1");
		//printf("New sock opened = %s\n", con_arr[k]->client_sock);
		for (list_node_t *n = country_dirs->head; n; n = n->next) {
			int *val = (int *) n->value;
			if (*val == k) // substitute the serial number of the monitor with the corresponding socket
				*val = con_arr[k]->client_sock;
		}
	}

	#ifdef DEBUG
	printf("ALL CONNECTIONS CREATED\n");
	#endif
	
    char virus[100];
    char country[100];
    uint8_t bloom_buf[BLOOM_SZ];
    for (int l = 0; l < num_monitors; l++) { // for each monitor
        strcpy(virus, ""); // init
        for (;;) { // receive all blooms
            read_msg(virus, SOCK_BUFFER_SZ, con_arr[l]->client_sock);
            if (!strcmp(virus, "STOP RECEIVING BLOOMS")) break;
            read_msg(country, SOCK_BUFFER_SZ, con_arr[l]->client_sock);
			//printf("Country = %s, virus = %s\n", country, virus);
            read_msg(bloom_buf, SOCK_BUFFER_SZ, con_arr[l]->client_sock);
            travel_monitor_insert_bloom(tm, virus, country, bloom_buf);
        }
    }
	char msg[100];
    for (int l = 0; l < num_monitors; l++) { // for each monitor
        strcpy(msg, "INITIALIZATION SEQUENCE COMPLETED"); // write INIT SEQ COMPLETE
        write_msg(msg, strlen(msg)+1, SOCK_BUFFER_SZ, con_arr[l]->client_sock);
    }


	char input[200];

	for (;;) {
        int status;
        fprintf(stdout, ">");
        if (fgets(input, sizeof(input), stdin)) {
            status = execute_command(tm, input, country_dirs, input_dir);
            if (status == EXIT_CMD) break;
            if (status == PARSE_ERROR)
                printf("==PARSE ERROR. NO COMMAND WAS EXECUTED\n");
        }
        else
            fprintf(stdout, "==ERROR: fgets error occured. Command not executed\n");
        fprintf(stdout, "\n");
	}


    for (int k = 0; k < num_monitors; k++) wait(NULL);

	// close all client sockets created
	for (int k = 0; k < num_monitors; k++) {
		//close(con_arr[k]->client_sock);
		connection_close(con_arr[k]);
		//printf("closing client sock\n");
	}
	for (int k = 0; k < n_subdirs; k++) 
		free(subdirs[k]);
	free(subdirs);
	travel_monitor_destroy(tm);
	for (list_node_t *n = country_dirs->head; n; n = n->next) {
        free((int *)n->value);
        free((char *)n->key);
    }
	list_delete_list(country_dirs);

	return EXIT_SUCCESS;
}








int country_dirs_cmp(void *v1, void *v2) {
    return strcmp((char *)v1, (char *)v2); // compare the dirnames
}

int isnumber(char* s) {
	for (int i = 0; i < strlen(s); i++)
		if (!isdigit(s[i])) // if there is atleast one non digit char
			return 0;
	return 1;
}

int get_npaths(list_t *l, int i) {
	int counter = 0;
	for (list_node_t *n = l->head; n; n = n->next) {
		int* mon = n->value;
		if (*mon == i)
			counter ++;

	}
	return counter;
}


int execute_command(travel_monitor_t * tm, char *cmd, list_t *country_dirs, char *input_dir) {
    delete_new_line(cmd);
    if (cmd[0] == '\0')  // in case no text was given
        return PARSE_ERROR; 
    char buff[100];
    int i = 0;
    int flag = 0;
	char* arg[6];
    memset(arg, 0, 6*sizeof(char*)); // empty the arguments
	for (char* token = strtok(cmd, " "); token != NULL; token = strtok (NULL, " ")) {
		if (i > 6) {
			printf("ERR: number of arguments exceeded!");
            return -1; // error parsing the cmd
			break;
		}
		arg[i] = token;
		i++;
	}

    if (!strcmp(arg[0], "/travelRequest")) {
        if (!isdate(arg[2])) {
            printf("==ERROR: Wrong date format\n");
            printf("Must be in DD-MM-YYYY format");
            return PARSE_ERROR;
        }
        travel_monitor_travel_request(tm, arg[1], arg[2], arg[3], arg[4], arg[5]);
        return PARSE_COMPLETE; 
    }
    else if (!strcmp(arg[0], "/travelStats")) {
        if (!isdate(arg[2]) || !isdate(arg[3])) { // if either one of the dates are wrong
            printf("==ERROR: Wrong date format\n");
            printf("Must be in DD-MM-YYYY format");
            return PARSE_ERROR;
        }
        travel_monitor_travel_stats(tm, arg[1], arg[2], arg[3], arg[4]);
        return PARSE_COMPLETE;
    }
    else if (!strcmp(arg[0], "/addVaccinationRecords")) {
		// not implemented
        list_node_t *n = list_search(country_dirs, arg[1]); // search for the country 
        if (n == NULL) {
            printf("==Error: Country %s does not exist.\n", arg[1]);
            return PARSE_COMPLETE;
        }
        int sock = *((int *)n->value); // get the process that is responsible for the country arg[1]
		int code = REQUESTING_BLOOMS_UPDATE;
		write_msg((byte_t *)&code, sizeof(int), SOCK_BUFFER_SZ, sock);
		write_msg((byte_t *)input_dir, strlen(input_dir)+1, SOCK_BUFFER_SZ, sock);
        // get the new bloomfilters
        connection_t *con;
        for (int i = 0; i < tm->sockets_array_sz; i++) {
            if (tm->sockets_array[i]->client_sock == sock) {
                con = tm->sockets_array[i];
                break;
            }
        }
        travel_monitor_update_blooms(tm, arg[1], con);
        return PARSE_COMPLETE;
    }
    else if (!strcmp(arg[0], "/searchVaccinationStatus")) {
		if (arg[1] == NULL) return PARSE_ERROR;
        travel_monitor_search_vaccination_status(tm, arg[1]);
        return PARSE_COMPLETE;
    }
    else if (!strcmp(arg[0], "/exit")) {
		broadcast_int(tm, TERMINATE_SERVER);
        return EXIT_CMD;
    }

    return PARSE_ERROR;
}
