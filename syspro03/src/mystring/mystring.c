

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mystring.h"


char *string_create(char *s) {
    char *str = (char *) malloc(sizeof(char) * (strlen(s) + 1));
    strcpy(str, s);
    return str;
}
