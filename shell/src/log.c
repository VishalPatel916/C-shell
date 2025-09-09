#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "parser.h"

int lo_g(char **cmd ,int *index,int *flag){
    int f=*flag;
    if(strcmp(tokens[1].value,"purge")==0){
        *index=0;
        *flag=0;
    }
    else if(strcmp(tokens[1].value,"execute")==0){
        int no_cmd=atoi(tokens[2].value);
        int imp_cmd=-1;
        
        if(f){
            imp_cmd=(*index)-no_cmd;
            imp_cmd=imp_cmd%15;
        }
        else{
            if(*index>=no_cmd){
                imp_cmd=(*index)-no_cmd;
            }
        }

        return imp_cmd;
    } 
    else{
        if(f){
            int n=*index;
            n--;
            for(int i=0;i<15;i++){
                printf("%s\n",cmd[(n%15)]);
                n--;
            }
        }
        else{
            for(int i=(*index)-1;i>=0;i--){
                printf("%s\n",cmd[i]);
            }
        }
    }
    return -1;
}