#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/wait.h>
#include "sequential.h"
#include "parser.h"
#include "arbitarycommand.h"
#include "redirection.h"
#include "activity.h"

int rc;
void background(int s,int e){

    rc=fork();
    if(rc==0){
        
        redirection(s);
        exit(0);
    }
    else if(rc>0){
        static int jo=0;
        jo++;
        jobs[job_count]=malloc(sizeof(job));
        jobs[job_count]->command=strdup(tokens[s].value);
        jobs[job_count]->pid=rc;
        jobs[job_count++]->state=Running;
        printf("[%d] %d\n",jo, rc);
    }
    else {
        perror("fork");
    }
}

void sequential(){
    int bg[tok_count-1];
    int b=0;
    for(int i=0;i<tok_count-1;i++){ 
        bg[i]=0;       
        if(tokens[i].type==TOK_AND){
            bg[b]=i;
        }
        if(tokens[i].type==TOK_COMMA || tokens[i].type==TOK_PIPE){
            b=i+1;
        }
    }
    for(int i=0;i<tok_count-1;i++){
        if(bg[i]!=0){
            background(i,bg[i]);
            i=bg[i];
        }
        else{
            redirection(i);
            
            while(tokens[i].type!=TOK_COMMA && tokens[i].type!=TOK_END){
                i++;
            }
        }
        
    }
}

