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
#include <semaphore.h>
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



// returns a record
record_t* parse_line(char *line, char *country) {


	delete_new_line(line); // delete '\n' at the end of line
    int sz = 8;
    char* array[sz];
    char* token;
    for (int i = 0; i < sz; i++) array[i] = NULL;

	int i = 0;
	for (token = strtok(line, " "); token != NULL; token = strtok (NULL, " ")) {
		array[i] = token;
		i++;
	}

	record_t *r = record_create(array[0], array[1], array[2], array[3],
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


monitor_t *monitor_create() {
    monitor_t *m = malloc(sizeof(monitor_t));
    m->countrieslist = list_create(countries_cmp, country_print);
    m->accepted = m->rejected = 0;
    // unassigned readfd and writefd.
    // will be assigned in the child_server socket creation
    //m->readfd = -1;
    //m->writefd = -1;
    return m;
}

void monitor_set_socket(monitor_t *m, int sock) {
    m->readfd = m->writefd = sock;
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
        write_msg(msg, strlen(msg)+1, SOCK_BUFFER_SZ, m->writefd);
        record_t *r = (record_t *) rec_node->value;
        sprintf(msg, "%s %s %s %s %s", r->citizen_id, r->first_name, r->last_name
                                            , r->last_name, r->country);

        write_msg(msg, strlen(msg)+1, SOCK_BUFFER_SZ, m->writefd); // send the first line 

        sprintf(msg, "AGE %d", r->age);
        write_msg(msg, strlen(msg)+1, SOCK_BUFFER_SZ, m->writefd); // send the line with the age
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
                write_msg(msg, strlen(msg)+1, SOCK_BUFFER_SZ, m->writefd);
                continue;
            }
            else {
                sprintf(msg, "%s NOT YET VACCINATED", r2->virus_name);
                write_msg(msg, strlen(msg)+1, SOCK_BUFFER_SZ, m->writefd);
                continue;
            }
            
        }
        break;
        
    }    
    if (found) {
        char msg[] = "FINISH OF VACCINATION STATUS";
        write_msg(msg, strlen(msg)+1, SOCK_BUFFER_SZ, m->writefd);
    }
    else {
        char msg[] = "NO";
        write_msg(msg, strlen(msg)+1, SOCK_BUFFER_SZ, m->writefd);
    }
    return found;
}




int filenames_cmp(void *f1, void *f2) {
    // the keys are only strings, so compare them
    return strcmp((char *)f1, (char *)f2);
}

void monitor_update_blooms(monitor_t *m, list_t *filelist
                          , sem_t *empty, sem_t *mutex, sem_t *full, cyclic_buf_t *cb) {



    char input_dir[150];
    read_msg(input_dir, SOCK_BUFFER_SZ, m->readfd); // read the input_dir to contruct the paths
    char *countryname = malloc(sizeof(char)*100);
    read_msg(countryname, SOCK_BUFFER_SZ, m->readfd); // read the countryname

    list_node_t *n = list_search(m->countrieslist, countryname);
    char stop_updating[] = "STOP UPDATING BLOOMS";
    if (n == NULL) { // if country does not exist, then terminate
        write_msg(stop_updating, strlen(stop_updating)+1, SOCK_BUFFER_SZ, m->writefd);
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
            printf("%s\n", temp_file);
            // FILE *f = fopen(temp_file, "r");
            // char line[200];
            // while (fgets(line, sizeof(line), f)) {
            //     record_t *r = parse_line(line, countryname);
            //     if (r == NULL) 
            //         continue; // if parse error, then skip
            //     monitor_insert_record(m, countryname, r);
            // }
            // fclose(f);

            sem_wait(empty);
            sem_wait(mutex);
            
            if (!cyclic_buf_insert(cb, temp_file))
                printf("Cyc buf is full\n");
            printf("Inserted file %s\n", temp_file);
			sem_post(mutex);
			sem_post(full);
        }

        printf("sz = %d\n", cb->num_elements);
        for (int j = 0; j < n; j++) free(files[j]); // free the file array, because
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
                write_msg(virus_ptr, strlen(virus_ptr) + 1, SOCK_BUFFER_SZ, m->writefd);
                //write_msg(country_ptr, strlen(country_ptr) + 1, SOCK_BUFFER_SZ, m->writefd);
                write_msg(bitarr, BLOOM_SZ, SOCK_BUFFER_SZ, m->writefd);
            }
        }
        // send STOP UPDATING message
        write_msg(stop_updating, strlen(stop_updating)+1, SOCK_BUFFER_SZ, m->writefd);
    }
    


    free(countryname);
}
