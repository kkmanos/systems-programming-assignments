#include <stdio.h>
#include <sys/types.h>	     /* sockets */
#include <time.h>
#include <string.h>
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <unistd.h>          /* read, write, close */
#include <netdb.h>	         /* gethostbyaddr */
#include <stdlib.h>	         /* exit */
#include <string.h>	         /* strlen */
#include "sockets.h"



void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

connection_t *connection_create(int serv_port, char *serv_hostname) {
    connection_t *con = (connection_t *) malloc(sizeof(connection_t));
    con->server_port = serv_port;
    con->server_hostname = malloc(sizeof(char) * (strlen(serv_hostname)+1));
    strcpy(con->server_hostname, serv_hostname);

    struct sockaddr *serverptr = (struct sockaddr *) &con->server;


	/* Create socket */
    if ((con->client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	perror_exit("socket");

	/* Find server address */
    if ((con->rem = gethostbyname(serv_hostname)) == NULL) {	
	   herror("gethostbyname"); exit(1);
    }
    con->server.sin_family = AF_INET;       /* Internet domain */
    memcpy(&con->server.sin_addr, con->rem->h_addr, con->rem->h_length);
    con->server.sin_port = htons(con->server_port);         /* Server port */

    // retry for connection until connected
    //while (connect(con->client_sock, serverptr, sizeof(struct sockaddr_in)) < 0);
    int res;
    do {
        #ifdef DEBUG
        printf("Trying to connect...\n");
        sleep(1);
        #endif
        
        res = connect(con->client_sock, serverptr, sizeof(struct sockaddr_in));
        
    } while (res < 0);


    
    return con;

}


void connection_close(connection_t *con) {
    close(con->client_sock);
    free(con->server_hostname);
    free(con);
}


// returns a socket number in sock
// returns port number
server_info_t *start_tcp_server(int port, int connections_to_listen) { 
    server_info_t *sinfo = (server_info_t *) malloc(sizeof(server_info_t));
	struct hostent *rem;
	/* Find server address */

	if ((sinfo->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		perror_exit("socket");

    int enable = 1;
    if (setsockopt(sinfo->sock, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
        perror_exit("setsockopt(SO_REUSEADDR) failed");
    if (setsockopt(sinfo->sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        perror_exit("setsockopt(SO_REUSEADDR) failed");

	sinfo->serverptr = (struct sockaddr *)&sinfo->server;
	sinfo->server.sin_family = AF_INET;
    sinfo->server.sin_addr.s_addr = htonl(INADDR_ANY);
	sinfo->server.sin_port = htons(port);
    //printf("htons port : %d \n", server.sin_port);

	/* Bind socket to address */
	if (bind(sinfo->sock, sinfo->serverptr, sizeof(struct sockaddr_in)) < 0) perror_exit("bind");
	if (listen(sinfo->sock, connections_to_listen) < 0) perror_exit("listen");
	return sinfo;

}


int get_available_port(char *hostname) {
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_exit("socket");

    int port;
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr *) & server;
    struct hostent *rem;


	/* Find server address */
    if ((rem = gethostbyname(hostname)) == NULL) {	
	   herror("gethostbyname"); exit(1);
    }
    server.sin_family = AF_INET;       /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);

    do {
        port = (rand() % (65535 - 1024 + 1)) + 65535;
        server.sin_port = htons(port);         /* Server port */
        /* Initiate connection */
    } while (connect(sock, serverptr, sizeof(struct sockaddr_in)) < 0);
    close(sock);
    return port;

}


int min(int a, int b) { return (a < b) ? a : b; }

void write_msg(byte_t* msg, int msg_sz, int buff_sz, int writefd) {
   int sz = msg_sz;

   byte_t buff[buff_sz];
   int i = 0;

//   char msg_sz_int[10];
//   sprintf(msg_sz_int, "%d", msg_sz);
//   write(writefd,  msg_sz_int, strlen(msg_sz_int)+1);
   write(writefd, &msg_sz, sizeof(int)); // send the size of the message 
    

   byte_t *addr = msg;
   while (sz > 0) {
      int bytes_to_send = min(buff_sz, sz);
      
      memcpy(buff, addr, bytes_to_send);
      int bytes_writen = write(writefd, buff, bytes_to_send);
      //sz = sz - bytes_to_send;
      //addr = addr + bytes_to_send;
      sz = sz - bytes_writen;
      addr = addr + bytes_writen;
   }
}

void read_msg(byte_t* dest, int buff_sz, int readfd) {
   int i = 0;
   byte_t buff[buff_sz];
   int sz;
   read(readfd, &sz, sizeof(int)); // read the size of the message

//    char msg_sz_int[10];
//    read(readfd, msg_sz_int, strlen(msg_sz_int)+1);
//    sz = atoi(msg_sz_int);

   byte_t *addr = dest;
   while (sz > 0) {
      int bytes_to_read = min(buff_sz, sz);
      int bytes_read = read(readfd, buff, bytes_to_read);
      memcpy(addr, buff, bytes_read);
      sz = sz - bytes_read;
      addr = addr + bytes_read;
   }

}


int test_connection(char *hostname, int port) {
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_exit("socket");

    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr *) & server;
    struct hostent *rem;


	/* Find server address */
    if ((rem = gethostbyname(hostname)) == NULL) {	
	   herror("gethostbyname"); exit(1);
    }
    server.sin_family = AF_INET;       /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);

    server.sin_port = htons(port);         /* Server port */

    if (connect(sock, serverptr, sizeof(struct sockaddr_in)) < 0) {
        close(sock);
        return 0;
    }
    else {
        close(sock);
        return 1;
    }

}