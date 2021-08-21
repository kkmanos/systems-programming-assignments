#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// get an array of subdirs and files, of the start_dir directory
// pass the address of the returng subdir array.
// free the memory allocated
// it includes the "." and ".." directory
int get_subdirs(char* start_dir, char*** ret_subdir_array) {
       struct dirent **namelist;

       int n;
       n = scandir(start_dir, &namelist, NULL, alphasort);
       char** subdir_array = (char**) malloc(n*sizeof(char*)); // alloc n pointers to strings

       int length = n;
       if (n < 0)
           perror("scandir");
       else {
           while (n--) {
           	   size_t l = strlen(namelist[n]->d_name) + 1;
               subdir_array[n] = (char*)malloc(sizeof(char)*l);

               strcpy(subdir_array[n], namelist[n]->d_name);
               //printf("%s\n", subdir_array[n]);
               free(namelist[n]);
           }
           free(namelist);
       }
       *ret_subdir_array = subdir_array;
       return length;
}

void subdirs_destroy(int nsubdirs, char **subdir_array) {
    for (int i = 0; i < nsubdirs; i++)
        free(subdir_array[i]);
    free(subdir_array);
}

void print_subdirs(unsigned int n, char** subdirs) {
    printf("\n==SUBDIRS: \n");
    for (int i = 0; i < n; i++) {
        printf("%s\n", subdirs[i]);
    }
}