#include <stdio.h>
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
#include "namedpipes/named_pipes.h"
#include "dir_traversal/dir_traversal.h"
#include "bloom_filter/bloom_filter.h"
#include "globals.h"
#include "travel_monitor/travel_monitor.h"

// used for execute command returning flags
#define PARSE_ERROR -1
#define EXIT_CMD 0
#define PARSE_COMPLETE 1



uint8_t travel_mon_exit = 0;  // is set to 1 if SIGQUIT or SIGINT is received
uint8_t travel_mon_sig_chld = 0;
void travel_mon_handler(int sig) {
    switch (sig) {
        case SIGQUIT:
            travel_mon_exit = 1;
            break;
        case SIGINT:
            travel_mon_exit = 1;
            break;
        case SIGCHLD:
            travel_mon_sig_chld = 1;
            break;

    }
}


void int_to_str(char* buf, int num) {  // result returned in buf
    sprintf(buf, "%d", num);
}

int execute_command(travel_monitor_t * tm, char *cmd, list_t *country_dirs) {
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
        list_node_t *n = list_search(country_dirs, arg[1]); // search for the country 
        if (n == NULL) {
            printf("==Error: Country %s does not exist.\n", arg[1]);
            return PARSE_COMPLETE;
        }
        int pid = *((int *)n->value); // get the process that is responsible for the country arg[1]
        kill(pid, SIGUSR1); // send SIGUSR1 signal to parse the new files and update the data structs
        // get the new bloomfilters
        named_pipe_t np;
        for (int i = 0; i < tm->np_array_sz; i++) {
            if (tm->np_array[i].monitor_pid == pid) {
                np = tm->np_array[i];
                break;
            }
        }
        travel_monitor_update_blooms(tm, arg[1], np);

        return PARSE_COMPLETE;
    }
    else if (!strcmp(arg[0], "/searchVaccinationStatus")) {
        travel_monitor_search_vaccination_status(tm, arg[1]);
        return PARSE_COMPLETE;
    }
    else if (!strcmp(arg[0], "/exit")) {
        return EXIT_CMD;
    }

    return PARSE_ERROR;
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



int main(int argc, char** argv) {

    signal(SIGINT, travel_mon_handler);
    signal(SIGQUIT, travel_mon_handler);
    signal(SIGCHLD, travel_mon_handler);


    char *input_dir = malloc(sizeof(char) * 100);
    unsigned int num_monitors;
    char *bloom_size = NULL;
    char *buffer_size = NULL;
	if (argc != 9) {
		printf("==ERROR: Wrong number of arguments.\n");
        printf("Usage: \n./travelMonitor –m numMonitors -b bufferSize -s sizeOfBloom -i input_dir\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 1; i <= 8; i++) { // parsing command line args
		if (!strcmp(argv[i], "-i") && i+1 <= 8) { // if read "-i" and there is next arg
            strcpy(input_dir, argv[i+1]);
            DIR *d = opendir(input_dir);
            if (d != NULL)
                closedir(d);
            else if (ENOENT == errno) {
                printf("==Error: input directory \"%s\" does not exist\n", input_dir);
                exit(EXIT_FAILURE);
            }
		}

		if (!strcmp(argv[i], "-b") && i+1 <= 8) { // if read "-b" and there is another arg
			if (isnumber(argv[i+1])) { // if next arg is a number, then store it
                buffer_size = argv[i+1];  // store it as string
				BUFFER_SZ = (unsigned int) atoi(argv[i+1]);
			}
			else {
				printf("==ERROR: -b arg not a number\n");
                printf("Usage: \n./travelMonitor –m numMonitors -b bufferSize -s sizeOfBloom -i input_dir\n");
				exit(EXIT_FAILURE);
			}
		}

		if (!strcmp(argv[i], "-s") && i+1 <= 8) { // if read "-b" and there is another arg
			if (isnumber(argv[i+1])) { // if next arg is a number, then store it
                bloom_size = argv[i+1]; // store it as a string
				BLOOM_SZ = (unsigned int) atoi(argv[i+1]);
			}
			else {
				printf("==ERROR: -s arg not a number\n");
                printf("Usage: \n./travelMonitor –m numMonitors -b bufferSize -s sizeOfBloom -i input_dir\n");
				exit(EXIT_FAILURE);
			}
		}
		if (!strcmp(argv[i], "-m") && i+1 <= 8) { // if read "-b" and there is another arg
			if (isnumber(argv[i+1])) { // if next arg is a number, then store it
				num_monitors = (unsigned int) atoi(argv[i+1]);
			}
			else {
				printf("==ERROR: -s arg not a number\n");
                printf("Usage: \n./travelMonitor –m numMonitors -b bufferSize -s sizeOfBloom -i input_dir\n");
				exit(EXIT_FAILURE);
			}
		}
	}

    pid_t childpid = 20;
    char msg[1000]; 
    char toappend[50];
    uint8_t bloom_buf[BLOOM_SZ];

    int server_readfd, server_writefd;
    char server_read_path[100];
    char server_write_path[100];
    // initialize np array, with paths.
    named_pipe_t np_array[num_monitors];
    pid_t travel_pid = getpid();
 
    for (int k = 0; k < num_monitors; k++) {
        strcpy(server_read_path, "/tmp/r.");
        strcpy(server_write_path, "/tmp/w.");
        int_to_str(toappend, k);
        strcat(server_read_path, toappend);
        strcat(server_write_path, toappend);
        named_pipe_init(&np_array[k],  server_read_path, server_write_path, server_readfd, server_writefd);

        makefifo(np_array[k].read_path, np_array[k].write_path); // create the fifos
    }


    // creating the children
    int i;
    for (i = 0; i < num_monitors; i++) // only parent increments
        if ( (childpid = fork()) <= 0 ) // if its a child, then break
            break;
    
    // i is now a proccess number (from 0 to num_monitors) (not the pid)
    
    if (childpid == 0) { // if this is a child proc (Monitor proc)
        strcpy(server_read_path, "/tmp/r.");
        strcpy(server_write_path, "/tmp/w.");
        int_to_str(toappend, i); 
        strcat(server_read_path, toappend); // "/tmp/r." + i
        strcat(server_write_path, toappend);  // "/tmp/w." + i      
        int ret_ex = execl("./Monitor", "./Monitor", buffer_size, server_read_path, server_write_path, bloom_size, input_dir, (char*)NULL);
        if (ret_ex == -1)
            printf("ERR: Monitor not created.\n");
        exit(0);
    }
    else if (getpid() == travel_pid) { // if this is the travelMonitor proc
        // country_dirs list is used from the travelMonitor to know which process has a specific dirname 
        list_t *country_dirs = list_create(country_dirs_cmp, NULL); // {key: dirname, value: pid}
                                                                    // must free the memory for both key and value
        for (int k = 0; k < num_monitors; k++) {
            strcpy(server_read_path, "/tmp/r.");
            strcpy(server_write_path, "/tmp/w.");
            int_to_str(toappend, k);
            strcat(server_read_path, toappend);
            strcat(server_write_path, toappend);
            //named_pipe_init(&np_array[k],  server_read_path, server_write_path, server_readfd, server_writefd);
            // start server side for file descs
            start_server(np_array[k].read_path, np_array[k].write_path, BUFFER_SZ, 
                                                    &np_array[k].readfd, &np_array[k].writefd);
            read_msg((byte_t *)&np_array[k].monitor_pid, BUFFER_SZ, np_array[k].readfd); //receive the pid of the monitor
        }

        travel_monitor_t *tm = travel_monitor_create(np_array, num_monitors);
        char** subdirs;
        int n_subdirs = get_subdirs(input_dir, &subdirs);
        int p = 0;
        // send the subdirs with round robin order
        for (int k = 0; k < n_subdirs; k++) { // for each subdir
            // skip useless subdirs
            if (!strcmp(subdirs[k], ".") || !strcmp(subdirs[k], ".."))
                continue;
            if (p == num_monitors) p = 0; // go back to the first monitor

            write_msg(subdirs[k], strlen((char*)subdirs[k])+1, BUFFER_SZ, np_array[p].writefd);
            char *countrydir = malloc(sizeof(char) * (strlen(subdirs[k]) + 1));
            int *pidnum = malloc(sizeof(int));
            strcpy(countrydir, subdirs[k]);
            *pidnum = np_array[p].monitor_pid;
            list_insert(country_dirs, countrydir, pidnum); // insert {countrydir, pid number}
            p++;
        } 

        for (int l = 0; l < num_monitors; l++) { // end of reading the countries.
            strcpy(msg, "STOP SENDING DIRS");
            write_msg(msg, strlen((char*)msg) + 1, BUFFER_SZ, np_array[l].writefd);
        }

        char virus[100];
        char country[100];
        for (int l = 0; l < num_monitors; l++) { // for each monitor
            strcpy(virus, ""); // init
            for (;;) { // receive all blooms
                read_msg(virus, BUFFER_SZ, np_array[l].readfd);
                if (!strcmp(virus, "STOP RECEIVING BLOOMS")) break;
                read_msg(country, BUFFER_SZ, np_array[l].readfd);
                read_msg(bloom_buf, BUFFER_SZ, np_array[l].readfd);
                travel_monitor_insert_bloom(tm, virus, country, bloom_buf);
            }
        }

        for (int l = 0; l < num_monitors; l++) { // for each monitor
            strcpy(msg, "INITIALIZATION SEQUENCE COMPLETED"); // write INIT SEQ COMPLETE
            write_msg(msg, strlen(msg)+1, BUFFER_SZ, np_array[l].writefd);
        }

        //printf("ALL BLOOMS RECEIVED\n");

        char input[200];
        for (;;) {
            if (travel_mon_exit == 1) // if exit signal(SIGINT or SIGQUIT) was received
                break;
            if (travel_mon_sig_chld == 1) {
                int status;
                pid_t terminated_pid = wait(&status);
                //printf("===== TERMINATED PID: %d\n", terminated_pid);
                named_pipe_t *child_np;
                for (int i = 0; i < num_monitors; i++) { // get the read and write paths for the terminated monitor
                    if (tm->np_array[i].monitor_pid == terminated_pid) {
                        strcpy(server_read_path, tm->np_array[i].read_path);
                        strcpy(server_write_path, tm->np_array[i].write_path);
                        child_np = &tm->np_array[i];
                        break;
                    }
                }
                pid_t new_child = fork();   // fork a new child

                if (new_child == 0) {
                    int ret_ex = execl("./Monitor", "./Monitor", buffer_size, server_read_path, server_write_path, bloom_size, input_dir, (char*)NULL);
                    if (ret_ex == -1)
                        printf("ERR: Monitor not created.\n");
                    exit(0);
                }

                //sleep(2);
                close_server(child_np->readfd, child_np->writefd);
                start_server(child_np->read_path, child_np->write_path, BUFFER_SZ,
                                                                &child_np->readfd, &child_np->writefd);

                int buff; // useless
                read_msg((byte_t *) &buff, BUFFER_SZ, child_np->readfd); //receive the pid of the monitor
                child_np->monitor_pid = new_child;  // change the pid of the child in the np_array
                for (list_node_t *cdir = country_dirs->head; cdir; cdir = cdir->next) {
                    int *pid = ((int *)cdir->value);
                    if (*pid == terminated_pid) {
                        char *dirname = (char *)cdir->key;
                        *pid = new_child; // change pid in the directory
                        write_msg((byte_t *) dirname, strlen(dirname)+1, BUFFER_SZ, child_np->writefd); // send dir name
                    }
                }
                strcpy(msg, "STOP SENDING DIRS");
                write_msg(msg, strlen((char*)msg) + 1, BUFFER_SZ, child_np->writefd);
                strcpy(virus, ""); // init
                for (;;) { // just read all blooms
                    read_msg(virus, BUFFER_SZ, child_np->readfd);
                    if (!strcmp(virus, "STOP RECEIVING BLOOMS")) break;
                    read_msg(country, BUFFER_SZ, child_np->readfd);
                    read_msg(bloom_buf, BUFFER_SZ, child_np->readfd);
                    //travel_monitor_insert_bloom(tm, virus, country, bloom_buf);
                }
                strcpy(msg, "INITIALIZATION SEQUENCE COMPLETED"); // write INIT SEQ COMPLETE
                write_msg(msg, strlen(msg)+1, BUFFER_SZ, child_np->writefd);
                travel_mon_sig_chld = 0;  // reset flag
            }

            int status;
            fprintf(stdout, ">");
            if (fgets(input, sizeof(input), stdin)) {
                status = execute_command(tm, input, country_dirs);
                if (status == EXIT_CMD) break;
                if (status == PARSE_ERROR)
                    printf("==PARSE ERROR. NO COMMAND WAS EXECUTED\n");
            }
            else
                fprintf(stdout, "==ERROR: fgets error occured. Command not executed\n");
            fprintf(stdout, "\n");
        }



        broadcast_signal(tm, SIGQUIT);
        

        for (int k = 0; k < num_monitors; k++) wait(NULL);
        for (int k = 0; k < num_monitors; k++) close_server(np_array[k].readfd, np_array[k].writefd);
        
        for (list_node_t *n = country_dirs->head; n; n = n->next) {
            free((int *)n->value);
            free((char *)n->key);
        }
        subdirs_destroy(n_subdirs, subdirs); 
        travel_monitor_destroy(tm);
        list_delete_list(country_dirs);
    }

    for (int i = 0; i < num_monitors; i++)
        named_pipe_destroy(&np_array[i]);

    free(input_dir);

    
}