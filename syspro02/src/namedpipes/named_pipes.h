
#ifndef __NAMED_PIPES__
#define __NAMED_PIPES__
#include <stdint.h>
typedef struct named_pipe {
    // server's info (travelMonitor)
    int readfd;  // read fd of travelMonitor
    int writefd;
    char* read_path;  // read path of travelMonitor
    char* write_path;
    int monitor_pid;
} named_pipe_t;


int makefifo(char *read_path, char *write_path);

void named_pipe_init(named_pipe_t* m, char* read_path, char* write_path, int readfd, int writefd);

void named_pipe_destroy(named_pipe_t* m);

int start_server(char* read_path, char* write_path, unsigned int buffsize, 
                                                    int *readfd, int *writefd);
int start_client(char* read_path, char* write_path, unsigned int buffsize, 
                                                    int *readfd, int *writefd);
int close_server(int readfd, int writefd);
int close_client(char* read_path, char* write_path, int readfd, int writefd);

typedef uint8_t byte_t;

void write_msg(byte_t* msg, int msg_sz, int buff_sz, int writefd);
void read_msg(byte_t* dest, int buff_sz, int readfd);

#endif