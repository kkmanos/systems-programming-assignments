#include  <sys/types.h>
#include  <sys/stat.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include "../bloom_filter/bloom_filter.h"
#include "monitor.h"
#include "../namedpipes/named_pipes.h"
#include "../country/country.h"
#include "../generic_list/generic_list.h"
#include "../tuple/tuple.h"
#include "../dir_traversal/dir_traversal.h"
#include "../globals.h"
#include "../virus/virus.h"
#include "../generic_list/generic_list.h"
#include "../request/request.h"

// global variables for signal receiving
uint8_t rcvd_sigusr2 = 0;
uint8_t rcvd_sigusr1 = 0;
uint8_t rcvd_sigquit = 0;
uint8_t rcvd_sigint = 0;



void handler(int sig) {

    // while this monitor is proccessing a request, wait
    // until all flags are set to 0.
    while (rcvd_sigint || rcvd_sigquit || rcvd_sigusr1 || rcvd_sigusr2);

    switch (sig) {
        case SIGUSR2:
            rcvd_sigusr2 = 1;
            break;
        case SIGUSR1:
            rcvd_sigusr1 = 1;
            break;
        case SIGQUIT:
            rcvd_sigquit = 1;
            break;
        case SIGINT:
            rcvd_sigint = 1;
            break;
    }
}

void reset_all_sig_flags() {
    rcvd_sigint = rcvd_sigusr1 = rcvd_sigusr2 = rcvd_sigquit = 0;
}

// returns a record
record_t* parse_line(char *line, char *country) {


	int i = 0;
	delete_new_line(line); // delete '\n' at the end of line
    char* array[8];
    char* token;
    for (int i = 0; i < 8; i++) array[i] = NULL;

	for (token = strtok(line, " "); token != NULL; token = strtok (NULL, " ")) {
		array[i] = token;
		i++;
	}

	record_t *r = record_create(array[0], array[1], array[2], country,
										atoi(array[4]), array[5], 
										!strcmp(array[6], "YES") ? 1 : 0,
										array[7]);
    return r;
}


void int_to_str(char* buf, int num) {  // result returned in buf
    sprintf(buf, "%d", num);
}
int countries_cmp(void * c1, void *c2) {
    return strcmp(c1, c2); // c1 and c2 are keys : char *
}

void country_print(FILE * f, void *val) {
    country_t *c = (country_t *) val;
    fprintf(f, "country name: %s\n", c->name);
}


monitor_t *monitor_create(int readfd, int writefd) {
    monitor_t *m = malloc(sizeof(monitor_t));
    m->countrieslist = list_create(countries_cmp, country_print);
    m->accepted = m->rejected = 0;
    m->readfd = readfd;
    m->writefd = writefd;
    return m;
}


void monitor_destroy(monitor_t *m) {
    for (list_node_t *cnode = m->countrieslist->head; cnode; cnode = cnode->next) {
        country_t *c = (country_t *) cnode->value;
        country_destroy(c);
    }
    list_delete_list(m->countrieslist); 
    free(m);
}

void monitor_insert_record(monitor_t *m, char *country, record_t *r) {
    
    list_node_t *n = list_search(m->countrieslist, country);
    country_t *c = NULL;
    if (n == NULL) { // if country not in countrieslist
        c = country_create(country);
        n = list_insert(m->countrieslist, c->name, c);
    }
    c = (country_t *) n->value; 

    country_insert_record(c, r);

}


char *monitor_get_date_vaccinated(monitor_t *m, char *id, char *virus, char *country) {
    list_node_t *country_n = list_search(m->countrieslist, country); 
    if (country_n == NULL) return NULL; // country does not exist

    country_t *c = (country_t *) country_n->value;
    tuple_t t = {id, virus};
    list_node_t *n = list_search(c->R, &t);
    if (n == NULL) return NULL; // record does not exist
    record_t *r = (record_t *) n->value;

    if (r->yN) // if vaccinated, return the date 
        return r->date_vaccinated;
    else
        return NULL;
}


// if found the citizen, send a YES and then send information about the citizen
// if citizen was not found, send a NO
// if citizen was found, send "FINISH OF VACCINATION STATUS" at the end
int monitor_search_vaccination_status(monitor_t *m, char *citizen_id) {

    int found = 0;
    // for n in countrieslist
    for (list_node_t *n = m->countrieslist->head; n; n = n->next) {
        country_t *c = (country_t *) n->value;
        tuple_t t = {citizen_id, NULL};
        list_node_t *rec_node = list_search(c->R, &t); // search for a record with this citizen_id
        if (rec_node == NULL) // if record not found, 
            continue;     // continue with the next country
        // else if record was found
        found = 1;
        char msg[100];
        strcpy(msg, "YES");
        write_msg(msg, strlen(msg)+1, BUFFER_SZ, m->writefd);
        record_t *r = (record_t *) rec_node->value;
        sprintf(msg, "%s %s %s %s %s", r->citizen_id, r->first_name, r->last_name
                                            , r->last_name, r->country);

        write_msg(msg, strlen(msg)+1, BUFFER_SZ, m->writefd); // send the first line 

        sprintf(msg, "AGE %d", r->age);
        write_msg(msg, strlen(msg)+1, BUFFER_SZ, m->writefd); // send the line with the age
        // for v in c->viruseslist   (we are traversing only to get all the possible virusnames for the citizen_id)
        for (list_node_t *v = c->viruseslist->head; v; v = v->next) {
            char *vname = (char *) v->key;
            tuple_t t2 = {citizen_id, vname};
            // search for a record with (citizen_id, vname) key
            list_node_t *record = list_search(c->R, &t2);
            if (record == NULL) // if does not exist 
                continue;        // continue with the next virusname
            record_t *r2 = (record_t *) record->value;
            if (r2->yN) { // if vaccinated, then send the date as well in one line
                sprintf(msg, "%s VACCINATED ON %s", r2->virus_name, r2->date_vaccinated);
                write_msg(msg, strlen(msg)+1, BUFFER_SZ, m->writefd);
                continue;
            }
            else {
                sprintf(msg, "%s NOT YET VACCINATED", r2->virus_name);
                write_msg(msg, strlen(msg)+1, BUFFER_SZ, m->writefd);
                continue;
            }
            
        }
        break;
        
    }    
    if (found) {
        char msg[] = "FINISH OF VACCINATION STATUS";
        write_msg(msg, strlen(msg)+1, BUFFER_SZ, m->writefd);
    }
    else {
        char msg[] = "NO";
        write_msg(msg, strlen(msg)+1, BUFFER_SZ, m->writefd);
    }
    return found;
}




int filenames_cmp(void *f1, void *f2) {
    // the keys are only strings, so compare them
    return strcmp((char *)f1, (char *)f2);
}

void monitor_update_blooms(monitor_t *m, list_t *filelist, char *input_dir) {
    char *countryname = malloc(sizeof(char)*100);
    read_msg(countryname, BUFFER_SZ, m->readfd); // read the countryname

    list_node_t *n = list_search(m->countrieslist, countryname);
    char stop_updating[] = "STOP UPDATING BLOOMS";
    if (n == NULL) { // if country does not exist, then terminate
        write_msg(stop_updating, strlen(stop_updating)+1, BUFFER_SZ, m->writefd);
    }
    else {
        char temp_dir[100];
        strcpy(temp_dir, input_dir);
        strcat(temp_dir, "/");
        strcat(temp_dir, countryname); // temp_dir = "../input_dir" + countryname
        char temp_file[100];
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
            //printf("%s\n", temp_file);
            FILE *f = fopen(temp_file, "r");
            char line[200];
            while (fgets(line, sizeof(line), f)) {
                record_t *r = parse_line(line, countryname);
                if (r == NULL) 
                    continue; // if parse error, then skip
                monitor_insert_record(m, countryname, r);
            }
            fclose(f);
        }
        for (int i = 0; i < n; i++) free(files[i]); // free the file array, because
        free(files);                // get_subdirs() will allocate new arrays

        char *country_ptr;
        char *virus_ptr;
        list_node_t *c = list_search(m->countrieslist, countryname); // get country node
        list_t *vlist = ((country_t *)c->value)->viruseslist; // get viruses list of countryname
        country_ptr = ((country_t *)c->value)->name;
        if (vlist->size != 0) {
            // for v in c->viruseslist
            for (list_node_t *v = vlist->head; v; v = v->next) {
                virus_ptr = ((virus_t *)v->value)->name;

                uint8_t *bitarr =  ((virus_t *)v->value)->bloom->bitarray;
                write_msg(virus_ptr, strlen(virus_ptr) + 1, BUFFER_SZ, m->writefd);
                //write_msg(country_ptr, strlen(country_ptr) + 1, BUFFER_SZ, m->writefd);
                write_msg(bitarr, BLOOM_SZ, BUFFER_SZ, m->writefd);
            }
        }
        // send STOP UPDATING message
        write_msg(stop_updating, strlen(stop_updating)+1, BUFFER_SZ, m->writefd);
    }
    


    free(countryname);
}

int main(int argc, char** argv) {

    signal(SIGUSR1, handler);
    signal(SIGUSR2, handler);
    signal(SIGQUIT, handler);
    signal(SIGINT, handler);


    BUFFER_SZ = atoi(argv[1]);
    BLOOM_SZ = atoi(argv[4]);
    char *client_write_path = (char*) malloc(sizeof(char) * (strlen(argv[2]) + 1));
    char *client_read_path = (char*) malloc(sizeof(char) * (strlen(argv[3]) + 1));
    char *input_dir = (char *) malloc(sizeof(char) * (strlen(argv[5]) + 1));


    strcpy(client_write_path, argv[2]);
    strcpy(client_read_path, argv[3]);
    strcpy(input_dir, argv[5]);

    int client_writefd, client_readfd;

    char buff[BUFFER_SZ];
    strcpy(buff, "");

    
    start_client(client_read_path, client_write_path, BUFFER_SZ, 
                &client_readfd, &client_writefd);


    monitor_t *M = monitor_create(client_readfd, client_writefd);

    //fprintf(stdout, "Started client for pid: %d\n", getpid());
    char country[1000];
    char **files;
    char temp_dir[200];
    strcpy(temp_dir, input_dir);
    char line[200];
    char temp_file[100];
    strcpy(temp_file, ""); // init temp_file

    list_t *filelist = list_create(filenames_cmp, NULL); // {key : filename, value : dirname }
                                                        // must free the memory for both value and key
    int pid = getpid();
    write_msg((byte_t *)&pid, sizeof(int), BUFFER_SZ, client_writefd);
    for (;;) { // reading countries subdirs and addding them to M 
        read_msg(country, BUFFER_SZ, client_readfd); // msg is a country name
        if (strcmp(country, "STOP SENDING DIRS")) {
            strcat(temp_dir, "/");
            strcat(temp_dir, country);
            //fprintf(stdout, "Received: %s\n", temp_dir);
        
            int n = get_subdirs(temp_dir, &files);
            for (int i = 0; i < n; i++) { // for each file of temp_dir
                // in case we get . or ..  ==> we skip the filename
                if (!strcmp(files[i], ".") || !strcmp(files[i], ".."))
                    continue;
                char *filename = malloc(sizeof(char) * (strlen(files[i])+1)); // will be freed in list destruction
                char *dirname = malloc(sizeof(char) * (strlen(country)+1)); // will be freed in list destruction
                strcpy(filename, files[i]);
                strcpy(dirname, country);
                list_insert(filelist, filename, dirname); // insert {filename, dirname}

                //construct the path
                strcpy(temp_file, temp_dir); // temp_file = "../input_dir/country"
                strcat(temp_file, "/");
                strcat(temp_file, files[i]); // temp_file = "../input_dir/country/file_i"
                //printf("%s\n", temp_file);
                FILE *f = fopen(temp_file, "r");
                while (fgets(line, sizeof(line), f)) {
                    record_t *r = parse_line(line, country);
                    if (r == NULL) 
                        continue; // if parse error, then skip
                    monitor_insert_record(M, country, r);
                }
                fclose(f);
            }
            strcpy(temp_dir, input_dir); // restart the dir path with the root dir
            for (int i = 0; i < n; i++) free(files[i]); // free the file array, because
            free(files);                // get_subdirs() will allocate new arrays
        }
        else
            break;
    }



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
                    write_msg(virus_ptr, strlen(virus_ptr) + 1, BUFFER_SZ, client_writefd);
                    write_msg(country_ptr, strlen(country_ptr) + 1, BUFFER_SZ, client_writefd);
                    write_msg(bitarr, BLOOM_SZ, BUFFER_SZ, client_writefd);
                }
            }
        }
    }

    char *end_transmission = "STOP RECEIVING BLOOMS";
    write_msg(end_transmission, strlen(end_transmission) + 1, BUFFER_SZ, client_writefd);


    read_msg(line, BUFFER_SZ, client_readfd); // read 'INITIALIZATION SEQUENCE COMPLETED'



    for (;;) {
        pause(); // wait for a signal and a flag will be changed

        if (rcvd_sigusr2) { // then its REQUESTING DATE VACCINATED or SEARCH VACCINATION STATUS
            int code;
            read_msg((byte_t *)&code, BUFFER_SZ, client_readfd); // read code
            if (code == REQUESTING_DATE_VACCINATED) {
                char *id = malloc(sizeof(char)*10);
                char *virus = malloc(sizeof(char)*40);
                char *countryfrom = malloc(sizeof(char)*40);
                read_msg((byte_t *)id, BUFFER_SZ, client_readfd); // read id 
                read_msg((byte_t *)virus, BUFFER_SZ, client_readfd); // read virus name
                read_msg((byte_t *)countryfrom, BUFFER_SZ, client_readfd); // read countryfrom
                char *date = monitor_get_date_vaccinated(M, id, virus, countryfrom);
                if (date == NULL) {
                    write_msg((byte_t *)"NO", strlen("NO")+1, BUFFER_SZ, client_writefd);
                    M->rejected++;
                }
                else {
                    write_msg((byte_t *)"YES", strlen("YES")+1, BUFFER_SZ, client_writefd);
                    write_msg((byte_t *)date, strlen(date)+1, BUFFER_SZ, client_writefd);
                    M->accepted++;
                }
                free(id);
                free(virus);
                free(countryfrom);
            }
            if (code == REQUESTING_SEARCH_VACCINATION_STATUS) {
                char *citizen_id = malloc(sizeof(char)*10);
                read_msg(citizen_id, BUFFER_SZ, client_readfd); // get the citizen id
                monitor_search_vaccination_status(M, citizen_id);
                free(citizen_id);
            }
            reset_all_sig_flags();
        }
        else if (rcvd_sigusr1) { // if received SIGUSR1
            monitor_update_blooms(M, filelist, input_dir);
            reset_all_sig_flags();
        }
        else if (rcvd_sigquit || rcvd_sigint) {
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
            break;
        }
    }

    monitor_destroy(M);
    // destroy the filelist
    for (list_node_t *f = filelist->head; f; f = f->next) {
        free((char *)f->key);
        free((char *)f->value);
    } 
    list_delete_list(filelist);
    //printf("EXITIN monitor\n");
    close_client(client_read_path, client_write_path, client_readfd, client_writefd);
    free(client_read_path);
    free(client_write_path);
    exit(0);
}