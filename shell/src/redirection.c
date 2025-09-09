#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "redirection.h"
#include "parser.h"
#include "arbitarycommand.h"

int scan=0;
int terminal=1;

void redirection(int p){
    for(int i=p;i<tok_count ;i++){
        if(tokens[i].type==TOK_COMMA || tokens[i].type==TOK_AND){
            break;
        }
        if(tokens[i].type==TOK_PIPE){
            piping(p);
            return;
        }
    }
    int i=p,j=0;
    scan=dup(0);
    int f=0;
    while(i<tok_count){
        if(tokens[i].type==TOK_COMMA || tokens[i].type==TOK_AND){
            break;
        }
        if(tokens[i].type==TOK_INPUT){
            j=i;
            f=1;
        }
        i++;
    }
    int fd;
    if(f){
        fd=open(tokens[j+1].value,O_RDONLY);
        if(fd==-1){
            printf("No such file or directory\n");
            return;
        }
        dup2(fd,0);
        close(fd);
    }
    i=p,j=0,f=0;
    terminal=dup(1);
   
    while(i<tok_count){
        if(tokens[i].type==TOK_COMMA || tokens[i].type==TOK_AND){
            break;
        }
        if(tokens[i].type==TOK_OUTPUT || tokens[i].type==TOK_OUTPUT_APPEND ){
            j=i;
            f=1;
        }
        i++;
    }
    if(f){
        if(tokens[j].type==TOK_OUTPUT  ){
            fd=open(tokens[j+1].value,O_WRONLY | O_CREAT | O_TRUNC,0644);
        }
        else if(tokens[j].type==TOK_OUTPUT_APPEND ){
            fd=open(tokens[j+1].value,O_WRONLY | O_CREAT|O_APPEND,0644);
        }
             
        if(fd==-1){
            printf("No such file or directory\n");
            return;
        }
        dup2(fd,1);
        close(fd);
    }
    arbtry_cmd(p);
    dup2(terminal,1);
    dup2(scan,0);

}