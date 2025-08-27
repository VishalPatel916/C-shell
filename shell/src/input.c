#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "input.h"


char* input(){
    char* str=NULL;
    size_t size=0;
    //scanf(" %[^\n]",str);
    if(getline(&str,&size,stdin)==-1){
        printf("error in getline \n");
        return NULL;
    }

    int s=strlen(str);
    if(s>0 && str[s-1]=='\n'){
        str[s-1]='\0';
    }
    return str;
}