#ifndef _DIR_TRAVERSAL_
#define _DIR_TRAVERSAL_


int get_subdirs(char* start_dir, char*** ret_subdir_array);

void subdirs_destroy(int nsubdirs, char **subdir_array);
void print_subdirs(unsigned int n, char** subdirs);
#endif