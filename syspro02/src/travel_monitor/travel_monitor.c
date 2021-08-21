#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "travel_monitor.h"
#include "../generic_list/generic_list.h"
#include "../tuple/tuple.h"
#include "../bloom_filter/bloom_filter.h"
#include "../globals.h" // to get BLOOM_SZ
#include "../namedpipes/named_pipes.h"
#include "../request/request.h"




void print(FILE *out, void *v) { return; }


travel_monitor_t *travel_monitor_create(named_pipe_t *np_array, unsigned int np_array_sz) {
    travel_monitor_t *tm = (travel_monitor_t *) malloc(sizeof(travel_monitor_t));
    tm->blooms = list_create(tuple_cmp, print);
    tm->requests = list_create(requests_cmp, print);
    tm->np_array = np_array;
    tm->np_array_sz = np_array_sz;
    return tm;
}


void travel_monitor_destroy(travel_monitor_t *tm) {
    for (list_node_t *tmp = tm->blooms->head; tmp; tmp = tmp->next) {
        tuple_t *key = (tuple_t *) tmp->key;
        free(key->x);
        free(key->y);
        bf_destroy((bf_t *)tmp->value);
        tuple_destroy(key);
    }
    list_delete_list(tm->blooms);

    for (list_node_t *tmp = tm->requests->head; tmp; tmp = tmp->next) {
        // key should not be freed, because its a pointer to tmp->value
        request_destroy((request_t *)tmp->value);
    }
    list_delete_list(tm->requests);
    free(tm);
}

void travel_monitor_insert_bloom(travel_monitor_t *tm, char *virus, char *country, uint8_t *bitarray) {
    bf_t *bloom = bf_copy(bitarray, BLOOM_SZ); // allocate memory for a bloomfilter
    char *v = malloc(sizeof(char) * (strlen(virus)+1));
    char *c = malloc(sizeof(char) * (strlen(country)+1));
    strcpy(v, virus);
    strcpy(c, country);
    tuple_t *t = tuple_create(v, c);
    list_insert(tm->blooms, t, bloom); // insert (virus, country) : bf_t*
}






void travel_monitor_travel_request(travel_monitor_t *tm, char *id, char *date, char *from
                            , char *to, char *virus) {

    
    request_t *req = request_create(tm->requests->size+1, id, date, from, to, virus, 0); // initialize as rejected

    tuple_t t = {virus, from};
    list_node_t *n = list_search(tm->blooms, &t);
    if (n == NULL) { // if key {virus, from} does not exist, then dont accept it
        list_insert(tm->requests, &req->request_id, req); // insert the request in the request list
        printf("REQUEST REJECTED - YOUR ARE NOT VACCINATED\n");
        return;
    }

    bf_t *bloom = (bf_t *) n->value;
    if (bf_test(bloom, id) == 0) {
        list_insert(tm->requests, &req->request_id, req); // insert the request in the request list
        printf("REQUEST REJECTED - YOUR ARE NOT VACCINATED\n");
        return;
    }
    else { // if MAYBE vaccinated
        char date_vaccinated[12];
        char msg[12];
        int isvaccinated = 0;
        // ask the monitor
        broadcast_signal(tm, SIGUSR2); // prepare monitors to expect a request
        broadcast_int(tm, REQUESTING_DATE_VACCINATED);
        broadcast_string(tm, id);
        broadcast_string(tm, virus);
        broadcast_string(tm, from);
        for (int i = 0; i < tm->np_array_sz; i++) {
            read_msg(msg, BUFFER_SZ, tm->np_array[i].readfd);
            if (!strcmp(msg, "NO") && isvaccinated == 0)
                strcpy(date_vaccinated, "NO");
            else if (!strcmp(msg, "YES")) { // if returned yes
                isvaccinated = 1;
                read_msg(msg, BUFFER_SZ, tm->np_array[i].readfd); // get the date
                strcpy(date_vaccinated, msg);
            }
        }

        //char *date_vaccinated = monitor_get_date_vaccinated(id, virus, from);
        if (!isvaccinated) { // if not vaccinated
            list_insert(tm->requests, &req->request_id, req); // insert the request in the request list
            printf("REQUEST REJECTED - YOUR ARE NOT VACCINATED\n");
            return;
        }
        // else if vaccinated check if has vaccinated the last 6 months
        if (in_last_six_months(date, date_vaccinated)) {
            req->accepted = 1;
            list_insert(tm->requests, &req->request_id, req); // insert the request in the request list
            printf("REQUEST ACCEPTED - HAPPY TRAVELS\n");
            return;
        }
        else {
            list_insert(tm->requests, &req->request_id, req); // insert the request in the request list
            printf("REQUEST REJECTED - YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE\n");
            return;
        }

    }
}









void travel_monitor_travel_stats(travel_monitor_t *tm, char *virus_name, char *date1, char *date2, char *country) {
    unsigned int accepted = 0, rejected = 0;
    if (country == NULL) { // if country was not given
        // for rnode in tm->requests    
        for (list_node_t *rnode = tm->requests->head; rnode; rnode = rnode->next) {
            request_t *req = (request_t *) rnode->value;
            if (in_date(req->date, date1, date2)) { // if request is in date1..date2
                if (req->accepted)
                    accepted++;
                else
                    rejected++;
            }

        }
    }
    else { // if country was given
        for (list_node_t *rnode = tm->requests->head; rnode; rnode = rnode->next) {
            request_t *req = (request_t *) rnode->value;
            if (!strcmp(req->from, country)) { // for this specific country only
                if (in_date(req->date, date1, date2)) { // if request is in date1..date2
                    if (req->accepted)
                        accepted++;
                    else
                        rejected++;
                }

            }
        }
    }

    printf("TOTAL REQUESTS %d\n", accepted + rejected);
    printf("ACCEPTED %d\n", accepted);
    printf("REJECTED %d\n", rejected);
}


// if found the citizend with id: citizen_id, then must receive a YES to print the info
// else if not found, do nothing
void travel_monitor_search_vaccination_status(travel_monitor_t *tm, char *citizen_id) {
    broadcast_signal(tm, SIGUSR2); // prepare monitors to expect a request
    broadcast_int(tm, REQUESTING_SEARCH_VACCINATION_STATUS); // send request type
    broadcast_string(tm, citizen_id); // send the citizen id 
    char line[150];
    for (int i = 0; i < tm->np_array_sz; i++) { // for each monitor
        read_msg(line, BUFFER_SZ, tm->np_array[i].readfd); // get a YES or NO
        if (!strcmp(line, "NO")) // if returned NO, then continue with the next monitor
            continue;
        else if (!strcmp(line, "YES")) { // if returned YES
            for (;;) {
                read_msg(line, BUFFER_SZ, tm->np_array[i].readfd);
                if (!strcmp(line, "FINISH OF VACCINATION STATUS")) // if received :FINISH
                    break;
                else
                    fprintf(stdout, "%s\n", line);

            }
        }
    }
}

void travel_monitor_update_blooms(travel_monitor_t *tm, char *countryname, named_pipe_t np) {
    uint8_t *bitarr = malloc(sizeof(uint8_t) * BLOOM_SZ);
    char virusname[50];
    write_msg(countryname, strlen(countryname)+1, BUFFER_SZ, np.writefd); // send the country name
    for (;;) {
        // read virus
        read_msg(virusname, BUFFER_SZ, np.readfd);
        if (!strcmp(virusname, "STOP UPDATING BLOOMS"))
            break;
        // read bloom filter
        read_msg((byte_t *) bitarr, BUFFER_SZ, np.readfd);
        tuple_t t = {virusname, countryname};
        list_node_t *n = list_search(tm->blooms, &t); 
        if (n != NULL) { // if {virus, countryname} in tm->blooms
            bf_destroy((bf_t *)n->value); // destroy the old bloom
            // replace it with the new one
            n->value = bf_copy(bitarr, BLOOM_SZ);
        }
        else if (n == NULL) { // if {virus, countryname} not in tm->blooms
            char *virus = malloc(sizeof(char) * (strlen(virusname) + 1));
            char *country = malloc(sizeof(char) * (strlen(countryname) + 1));
            // add {(virus, country) : new bloomfilter} into the blooms list
            list_insert(tm->blooms, tuple_create(virus, country),  bf_copy(bitarr, BLOOM_SZ));
        }
    }
    free(bitarr);
}




void broadcast_string(travel_monitor_t *tm, char *msg) { // broadcasts to all open monitors the msg string
    for (int i = 0; i < tm->np_array_sz; i++) {
        write_msg((byte_t *)msg, strlen(msg) + 1, BUFFER_SZ, tm->np_array[i].writefd);
    }
}

void broadcast_int(travel_monitor_t *tm, int integer) {
    for (int i = 0; i < tm->np_array_sz; i++) {
        write_msg((byte_t *)&integer, sizeof(int), BUFFER_SZ, tm->np_array[i].writefd);
    }
}

void broadcast_signal(travel_monitor_t *tm, int sig) {
    for (int i = 0; i < tm->np_array_sz; i++)
        kill(tm->np_array[i].monitor_pid, sig);
}

