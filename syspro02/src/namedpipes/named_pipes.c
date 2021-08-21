#include  <sys/types.h>
#include  <sys/stat.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "named_pipes.h"
extern int errno;
#define PERMS   0666


void named_pipe_init(named_pipe_t* m, char* read_path, char* write_path, int readfd, int writefd) {
    m->readfd = readfd;
    m->writefd = writefd;

    m->read_path = (char*) malloc(sizeof(char) * (strlen(read_path)+1));
    m->write_path = (char*) malloc(sizeof(char) * (strlen(write_path)+1));
    strcpy(m->read_path, read_path);
    strcpy(m->write_path, write_path);
}

void named_pipe_destroy(named_pipe_t* m) {
    free(m->read_path);
    free(m->write_path);
    m->read_path = m->write_path = NULL;
}



int makefifo(char *read_path, char *write_path) {
	if ( (mkfifo(read_path, PERMS) < 0) && (errno != EEXIST) ) {
		 perror("server: can't create fifo");
		 return -1;
	}
	if ((mkfifo(write_path, PERMS) < 0) && (errno != EEXIST)) {
		 unlink(read_path);
		 perror("server: can't create fifo");
		 
		 return -1;
	}


}

int start_server(char* read_path, char* write_path, unsigned int buffsize, int *readfd, int *writefd) {
	char buff[buffsize];
	char errmesg[256];
	int n, fd;

	if ( (*readfd = open(read_path, O_RDONLY  ))  < 0)  {
		perror("server: can't open read fifo");
		return -1;
	}
 
	if ( (*writefd = open(write_path, O_WRONLY  ))  < 0)  {
		perror("server: can't open write fifo");
		return -1;
	}
}

int close_server(int readfd, int writefd) {
	close(readfd);
	close(writefd);
}



	  
int start_client(char* read_path, char* write_path, unsigned int buffsize, int *readfd, int *writefd) {

	char buff[buffsize];
	int n;
	/* Open the FIFOs.  We assume server has already created them.  */
	if ( (*writefd = open(write_path, O_WRONLY))  < 0)  {
		printf("client: cant open fifo in path %s", write_path);
		perror("client: can't open write fifo \n");
	}
	if ( (*readfd = open(read_path, O_RDONLY  ))  < 0)  {
		perror("client: can't open read fifo \n");
	}
}

int close_client(char* read_path, char* write_path, int readfd, int writefd) {

	if ( unlink(read_path) < 0) {
	 perror("client: can't unlink \n");
  }
	if ( unlink(write_path) < 0) {
	 perror("client: can't unlink \n");
  }

	close(readfd);
	close(writefd);
}



int min(int a, int b) { return (a < b) ? a : b; }

void write_msg(byte_t* msg, int msg_sz, int buff_sz, int writefd) {
   int sz = msg_sz;

   byte_t buff[buff_sz];
   int i = 0;
   
   write(writefd, &msg_sz, sizeof(int)); // send the size of the message 

   byte_t *addr = msg;
   while (sz > 0) {
      int bytes_to_send = min(buff_sz, sz);
      memcpy(buff, addr, bytes_to_send);
      write(writefd, buff, bytes_to_send);
      sz = sz - bytes_to_send;
      addr = addr + bytes_to_send;
   }
}

void read_msg(byte_t* dest, int buff_sz, int readfd) {
   int i = 0;
   byte_t buff[buff_sz];
   int sz;
   read(readfd, &sz, sizeof(int)); // read the size of the message

   byte_t *addr = dest;
   while (sz > 0) {
      int bytes_to_read = min(buff_sz, sz);
      read(readfd, buff, bytes_to_read);
      memcpy(addr, buff, bytes_to_read);
      sz = sz - bytes_to_read;
      addr = addr + bytes_to_read;
   }

}

