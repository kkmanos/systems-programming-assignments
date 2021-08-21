#include  <sys/types.h>
#include  <sys/stat.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <sys/wait.h>
#include "../bloom_filter/bloom_filter.h"
#include "monitor.h"
#include "../country/country.h"
#include "../generic_list/generic_list.h"
#include "../tuple/tuple.h"
#include "../dir_traversal/dir_traversal.h"
#include "../globals.h"
#include "../virus/virus.h"
#include "../generic_list/generic_list.h"
#include "../request/request.h"
#include "../sockets/sockets.h"
#include "../cyclic_buffer/cyclic_buffer.h"
#include "../mystring/mystring.h"

sem_t empty, mutex, full;

cyclic_buf_t *cb = NULL; // initialize cyclic buffer

int num_recs = 0;

void child_server(monitor_t *M, int newsock, char **paths, list_t *filelist);
int isnumber(char* s);

int string_cmp(void *a, void *b);
char *get_country_from_path_to_file(char *path);
void *thread_i(void *mon);
void sigchld_handler (int sig);
char *get_country_from_path(char *path); // from "/path/to/input_dir/country_dir" -> "country_dir"

int main(int argc, char **argv) {

    int max_arg_index = argc - 1;
    char *bloom_size;
    char *socket_buffer_size;
    unsigned int num_monitors = 0;
    unsigned int port;

	for (int i = 1; i <= max_arg_index; i++) { // parsing command line args
		if (!strcmp(argv[i], "-b") && i+1 <= max_arg_index) { // if read "-b" and there is another arg
			if (isnumber(argv[i+1])) { // if next arg is a number, then store it
                socket_buffer_size = argv[i+1];  // store it as string
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
		if (!strcmp(argv[i], "-p") && i+1 <= max_arg_index) {
			if (isnumber(argv[i+1])) { // if next arg is a number, then store it
				port = (unsigned int) atoi(argv[i+1]);
			}
			else {
				printf("==ERROR: -p arg not a number\n");
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
	sem_init(&mutex, 0, 1);
	sem_init(&empty, 0, CYCLIC_BUFFER_SZ);
	sem_init(&full, 0, 0);

	cb = cyclic_buf_create(CYCLIC_BUFFER_SZ); // initialize the global cyclic buffer


    const unsigned int npaths = argc-11;
    char *paths[npaths]; // array holding the paths
    int k = 0;
    for (int i = 11; i < argc; i++) { // read paths
        paths[k] = malloc(sizeof(char) * (strlen(argv[i]) + 1));
        strcpy(paths[k], argv[i]);
        //printf("path: %s\n", paths[k]);
        k++;
    }

	char temp_file[200];
    list_t *filelist = list_create(filenames_cmp, NULL); // {key : filename, value : dirname }
                                                        // must free the memory for both value and key
 	monitor_t *M = monitor_create();
	pthread_t threads_arr[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; i++)
		pthread_create(&threads_arr[i], NULL, thread_i, (void *) M); // pass the pointer to the monitor

	#ifdef DEBUG
	printf("After thread creation\n");
	#endif

	list_t *all_paths = list_create(string_cmp, NULL);
	int nfiles = 0;
	char **files; // no need to alloc. Only need to be freed, after call of get_subdirs
	for (int i = 0; i < npaths; i++) { // for each path
		int n = get_subdirs(paths[i], &files);
		char *country = get_country_from_path(paths[i]);
		for (int j = 0; j < n; j++) {
			if (!strcmp(files[j], ".") || !strcmp(files[j], ".."))
				continue;
			char *filename = malloc(sizeof(char) * (strlen(files[j])+1)); // will be freed in list destructor
			char *dirname = malloc(sizeof(char) * (strlen(country)+1));
			strcpy(filename, files[j]);
			strcpy(dirname, country);
			list_insert(filelist, filename, dirname); // insert {filename, dirname}

			// construct fullpath to the file
			strcpy(temp_file, paths[i]);
			strcat(temp_file, "/");
			strcat(temp_file, files[j]);

			sem_wait(&empty);
			sem_wait(&mutex);
			nfiles++;
			//printf("recs = %d\n", nfiles);
			if (cyclic_buf_insert(cb, temp_file) == 0) {
				printf("Buffer is fulllll\n");
			 	exit(-1);
			}
			sem_post(&mutex);
			sem_post(&full);

		}
        for (int j = 0; j < n; j++) free(files[j]); // free the file array
        free(files);  
		free(country);
		country = NULL;
		
	}



	//while (!(cb->num_elements == 0)); // wait until init sequence is completed
	#ifdef DEBUG
	printf("Init sequence completed with %d, recs = %d\n", nfiles, num_recs);
	#endif


	int max_num_connections = 1;
	int sock;
	//server_info_t *sinfo = start_tcp_server(port, max_num_connections);
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		perror_exit("socket");

    int enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
         perror_exit("setsockopt(SO_REUSEADDR) failed");
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
        perror_exit("setsockopt(SO_REUSEADDR) failed");
 

	struct hostent *rem;
    struct sockaddr *serverptr;
    struct sockaddr_in server;
	serverptr = (struct sockaddr *)&server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(port);
    //printf("htons port : %d \n", server.sin_port);

	/* Bind socket to address */
	if (bind(sock, serverptr, sizeof(server)) < 0) perror_exit("bind");
	if (listen(sock, max_num_connections) < 0) perror_exit("listen");
	
	#ifdef DEBUG
	printf("Server listening...\n");
	#endif

	int newsock;
	struct sockaddr_in client;
	socklen_t clientlen;
	struct sockaddr *clientptr=(struct sockaddr *)&client;
	int num_connections = 0;

	for (;;) {
		if (num_connections == max_num_connections) break;
		/* accept connection */
		if ((newsock = accept(sock, clientptr, &clientlen)) < 0) perror_exit("accept");
    	if ((rem = gethostbyaddr((char *) &client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL) {
   	    	herror("gethostbyaddr"); 
			exit(1);
		}
		#ifdef DEBUG
		printf("Con accepted\n");
		#endif

		num_connections++;

		switch (fork()) {    /* Create child for serving client */
		case -1:     /* Error */
			perror("fork"); break;
		case 0:	     /* Child process */
			close(sock); child_server(M, newsock, paths, filelist);
			exit(0);
		}
		close(newsock); /* parent closes socket to client */
			/* must be closed before it gets re-assigned */
	}	



	for (int i = 0; i < num_connections; i++) // wait for the child processes to end
		wait(NULL);



    char logfile[20];
    sprintf(logfile, "log_file.%d", getpid());
    FILE *out = fopen(logfile, "w+");
    for (list_node_t *cnode = M->countrieslist->head; cnode; cnode = cnode->next) {
        fprintf(out, "%s\n", (char *)cnode->key); // print the name for each country
    }

    fprintf(out, "TOTAL TRAVEL REQUESTS %d\n", M->rejected + M->accepted);
    fprintf(out, "ACCEPTED %d\n", M->accepted);
    fprintf(out, "REJECTED %d\n", M->rejected);
    fclose(out);


	for (int i = 0; i < npaths; i++)
		free(paths[i]);


	for (int i = 0; i < NUM_THREADS; i++) // kill all the threads
		pthread_cancel(threads_arr[i]);

	#ifdef DEBUG
	printf("Threads canceled\n");
	#endif
	cyclic_buf_destroy(cb);
	sem_destroy(&empty);
	sem_destroy(&full);
	sem_destroy(&mutex);

    return EXIT_SUCCESS;
}

void child_server(monitor_t *M, int newsock, char **paths, list_t *filelist) {

	char msg[110];
	monitor_set_socket(M, newsock);


    // SENDING THE BLOOMS
    char *country_ptr;
    char *virus_ptr;
    if (M->countrieslist->size != 0) {
        // for c in countrieslist
        for (list_node_t *c = M->countrieslist->head; c; c = c->next) {

            list_t *vlist = ((country_t *)c->value)->viruseslist;
            country_ptr = ((country_t *)c->value)->name;
            if (vlist->size != 0) {
                // for v in c->viruseslist
                for (list_node_t *v = vlist->head; v; v = v->next) {
                    virus_ptr = ((virus_t *)v->value)->name;

                    uint8_t *bitarr =  ((virus_t *)v->value)->bloom->bitarray;
                    write_msg(virus_ptr, strlen(virus_ptr) + 1, SOCK_BUFFER_SZ, newsock);
                    write_msg(country_ptr, strlen(country_ptr) + 1, SOCK_BUFFER_SZ, newsock);
                    write_msg(bitarr, BLOOM_SZ, SOCK_BUFFER_SZ, newsock); 
                }
            }
        }
    }

    char *end_transmission = "STOP RECEIVING BLOOMS";
    write_msg(end_transmission, strlen(end_transmission) + 1, SOCK_BUFFER_SZ, newsock);
    read_msg(msg, SOCK_BUFFER_SZ, newsock); // read 'INITIALIZATION SEQUENCE COMPLETED'


	int code;
	for (;;) {
		read_msg((byte_t *)&code, SOCK_BUFFER_SZ, newsock);
		if (code == REQUESTING_DATE_VACCINATED) {
            char *id = malloc(sizeof(char)*10);
            char *virus = malloc(sizeof(char)*40);
            char *countryfrom = malloc(sizeof(char)*40);
            read_msg((byte_t *)id, SOCK_BUFFER_SZ, newsock); // read id 
            read_msg((byte_t *)virus, SOCK_BUFFER_SZ, newsock); // read virus name
            read_msg((byte_t *)countryfrom, SOCK_BUFFER_SZ, newsock); // read countryfrom
            char *date = monitor_get_date_vaccinated(M, id, virus, countryfrom);
            if (date == NULL) {
                write_msg((byte_t *)"NO", strlen("NO")+1, SOCK_BUFFER_SZ, newsock);
                M->rejected++;
            }
            else {
                write_msg((byte_t *)"YES", strlen("YES")+1, SOCK_BUFFER_SZ, newsock);
                write_msg((byte_t *)date, strlen(date)+1, SOCK_BUFFER_SZ, newsock);
                M->accepted++;
            }
            free(id);
            free(virus);
            free(countryfrom);
		}
        else if (code == REQUESTING_BLOOMS_UPDATE) {

			pthread_t threads_arr[NUM_THREADS];
			for (int i = 0; i < NUM_THREADS; i++)
			 	pthread_create(&threads_arr[i], NULL, thread_i, (void *) M); // pass the pointer to the monitor

			#ifdef DEBUG
			int e, f, m;
			sem_getvalue(&empty, &e); 
			sem_getvalue(&full, &f); 
			sem_getvalue(&mutex, &m);
			printf("emptysem = %d, fullsem = %d, mutexsem = %d\n", e, f, m);
			#endif
            //monitor_update_blooms(M, filelist, &empty, &mutex, &full, cb);
			char input_dir[150];
			read_msg(input_dir, SOCK_BUFFER_SZ, M->readfd); // read the input_dir to contruct the paths
			char *countryname = malloc(sizeof(char)*100);
			read_msg(countryname, SOCK_BUFFER_SZ, M->readfd); // read the countryname

			list_node_t *n = list_search(M->countrieslist, countryname);
			char stop_updating[] = "STOP UPDATING BLOOMS";
			if (n == NULL) { // if country does not exist, then terminate
				write_msg(stop_updating, strlen(stop_updating)+1, SOCK_BUFFER_SZ, M->writefd);
			}
			else {
				char temp_dir[150];
				strcpy(temp_dir, input_dir);
				strcat(temp_dir, "/");
				strcat(temp_dir, countryname); // temp_dir = "../input_dir" + countryname
				char temp_file[120];
				char **files;
				int n = get_subdirs(temp_dir, &files);
				for (int i = 0; i < n; i++) { // for each file of "../input_dir/countryname" 
					// in case we get . or ..  ==> we skip the filename
					if (!strcmp(files[i], ".") || !strcmp(files[i], ".."))
						continue;
					
					list_node_t *filenode = list_search(filelist, files[i]);
					if (filenode != NULL) // if files[i] already exists in filelist
						continue;       // then skip it

					char *c = malloc(sizeof(char) * (strlen(countryname)+1)); // will be freed in list destruction
					char *ff = malloc(sizeof(char) * (strlen(files[i]) + 1)); // will be freed in list destruction
					strcpy(c, countryname);
					strcpy(ff, files[i]);
					list_insert(filelist, ff, c); // insert the new file in the filelist as {filename : countryname}

					char *filename = malloc(sizeof(char) * (strlen(files[i])+1));
					char *dirname = malloc(sizeof(char) * (strlen(countryname)+1));
					strcpy(filename, files[i]);
					strcpy(dirname, countryname);
					list_insert(filelist, filename, dirname); // insert {filename, dirname}

					//construct the path
					strcpy(temp_file, temp_dir); // temp_file = "../input_dir/country"
					strcat(temp_file, "/");
					strcat(temp_file, files[i]); // temp_file = "../input_dir/country/file_i"
					#ifdef DEBUG
					printf("%s\n", temp_file);
					#endif

					// FILE *f = fopen(temp_file, "r");
					// char line[200];
					// while (fgets(line, sizeof(line), f)) {
					//     record_t *r = parse_line(line, countryname);
					//     if (r == NULL) 
					//         continue; // if parse error, then skip
					//     monitor_insert_record(m, countryname, r);
					// }
					// fclose(f);

					sem_wait(&empty);
					sem_wait(&mutex);


					int res = cyclic_buf_insert(cb, temp_file);
					#ifdef DEBUG	
					printf("cyc-buf-res = %d\n", res);
					printf("Inserted file %s\n", temp_file);
					#endif

					sem_post(&mutex);
					sem_post(&full);
				}

				for (int j = 0; j < n; j++) free(files[j]); // free the file array, because
				free(files);                // get_subdirs() will allocate new arrays

				char *country_ptr;
				char *virus_ptr;
				list_node_t *c = list_search(M->countrieslist, countryname); // get country node
				list_t *vlist = ((country_t *)c->value)->viruseslist; // get viruses list of countryname
				country_ptr = ((country_t *)c->value)->name;
				if (vlist->size != 0) {
					// for v in c->viruseslist
					for (list_node_t *v = vlist->head; v; v = v->next) {
						virus_ptr = ((virus_t *)v->value)->name;

						uint8_t *bitarr =  ((virus_t *)v->value)->bloom->bitarray;
						write_msg(virus_ptr, strlen(virus_ptr) + 1, SOCK_BUFFER_SZ, M->writefd);
						//write_msg(country_ptr, strlen(country_ptr) + 1, SOCK_BUFFER_SZ, m->writefd);
						write_msg(bitarr, BLOOM_SZ, SOCK_BUFFER_SZ, M->writefd);
					}
				}
				// send STOP UPDATING message
				write_msg(stop_updating, strlen(stop_updating)+1, SOCK_BUFFER_SZ, M->writefd);
			}
			


			free(countryname);
			for (int i = 0; i < NUM_THREADS; i++) // kill all the threads
			 	pthread_cancel(threads_arr[i]);
        }
		else if (code == REQUESTING_SEARCH_VACCINATION_STATUS) {
            char *citizen_id = malloc(sizeof(char)*10);
            read_msg(citizen_id, SOCK_BUFFER_SZ, newsock); // get the citizen id
            monitor_search_vaccination_status(M, citizen_id);
            free(citizen_id);
		}
		else if (code == TERMINATE_SERVER)  {
			break;
		}
	}

	close(newsock);	  /* Close socket */
}

int isnumber(char* s) {
	for (int i = 0; i < strlen(s); i++)
		if (!isdigit(s[i])) // if there is atleast one non digit char
			return 0;
	return 1;
}

char *get_country_from_path(char *path) {
	char temp_path[strlen(path)+1];
	strcpy(temp_path, path);
	char *res = malloc(sizeof(char) * (strlen(path) + 1));

   for (char *token = strtok(temp_path, "/"); token; token = strtok(NULL, "/")) {
	   strcpy(res, token);
   }
   return res;
}

void *thread_i(void *mon) {
	monitor_t *M = (monitor_t *) mon;

	for (;;) {

		#ifdef DEBUG
		printf("Thread reseted\n");
		int e, fu, m;
		sem_getvalue(&empty, &e); 
		sem_getvalue(&full, &fu); 
		sem_getvalue(&mutex, &m);
		printf("emptysem = %d, fullsem = %d, mutexsem = %d\n", e, fu, m);
		#endif
		sem_wait(&full);
		#ifdef DEBUG
		printf("Unblock from full\n");
		#endif
		sem_wait(&mutex);

		char *path_to_file = cyclic_buf_pop(cb);
		#ifdef DEBUG
		printf("Poped path = %s\n", path_to_file);
		#endif

		if (path_to_file == NULL) {
			printf("====BUFF IS EMPTY\n");
			sem_post(&mutex);
			sem_post(&empty);
			continue;
		}

		char *country = get_country_from_path_to_file(path_to_file);
		FILE *f = fopen(path_to_file, "r");
		if (f == NULL) {
			printf("==File open failed\n");
			sem_post(&mutex);
			sem_post(&empty);
			continue;
		}
		char line[200];

		
        while (fgets(line, sizeof(line), f)) {
			#ifdef DEBUG
			//printf("%s\n", line);
			#endif
			record_t *r = parse_line(line, country);
			if (r == NULL) 
                continue; // if parse error, then skip
			num_recs++;
            monitor_insert_record(M, country, r);
		} 
        fclose(f);

		free(country);
		sem_post(&mutex);
		sem_post(&empty);


	}


}

char *get_country_from_path_to_file(char *path) {
	char temp_path[strlen(path)+1];
	strcpy(temp_path, path);
	char *cur = malloc(sizeof(char) * (strlen(path) + 1));	
	char *prev = malloc(sizeof(char) * (strlen(path) + 1));	
	for (char *token = strtok(temp_path, "/"); token; token = strtok(NULL, "/")) {

	   strcpy(prev, cur);
	   strcpy(cur, token);
	}
	free(cur);
	return prev;
}

/* Wait for all dead child processes */
void sigchld_handler (int sig) {
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

int string_cmp(void *a, void *b) {
	return strcmp(a, b);
}
