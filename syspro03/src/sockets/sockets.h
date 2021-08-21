
#ifndef SOCKETS_H
#define SOCKETS_H
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */


typedef struct connection_t { // is used by a client 
    int client_sock;
    int server_port;
    char *server_hostname;
    struct sockaddr_in server;
    struct hostent *rem;
} connection_t;

typedef struct server_info_t {
    struct sockaddr *serverptr;
    struct sockaddr_in server;
    int sock;
} server_info_t;



connection_t *connection_create(int serv_port, char *serv_hostname);
void connection_close(connection_t *con);

server_info_t *start_tcp_server(int port, int connections_to_listen);


int get_available_port(char *hostname);

void perror_exit(char *message);

typedef uint8_t byte_t;

void write_msg(byte_t* msg, int msg_sz, int buff_sz, int writefd);
void read_msg(byte_t* dest, int buff_sz, int readfd);



int test_connection(char *hostname, int port);

#endif