#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <fcntl.h>
#include <string.h>
#include<sys/wait.h>
#include <limits.h>
#include "parser.h"

void arbtry_cmd(){
    // char *cmd[tok_count];
    // for(int i=0;i<tok_count;i++){
    //     if(tokens[i].type==TOK_END){
    //         cmd[i]=NULL;
    //     }
    //     else{
    //         cmd[i]=tokens[i].value;
    //     }       

    // }

    // int pid=fork();
    // if(pid<0){
    //     printf("fork failed");
    //     return;
    // }
    // else if(pid==0){
    //     execvp(cmd[0],cmd);
    // }
    // else {
    //     int status;
    //     waitpid(pid,&status,0);

    // }


    if(strcmp(tokens[0].value,"sleep")==0){
        if(tok_count>2){
            int s=atoi(tokens[1].value);
            sleep(s);
        }
    }
    else if(strcmp(tokens[0].value,"cat")==0){
        if(tok_count>2){
            
            int scan=dup(0);
            char line[PATH_MAX];
            int file=open(tokens[1].value,O_RDONLY);
            if(file==-1){
                printf("No such file or directory\n");
                return;
            }
            dup2(file,0);
            while(fgets(line,sizeof(line),stdin)!=NULL){
                
                printf("%s\n",line);
            }
            dup2(scan,0);
            clearerr(stdin);
            close(file);
            close(scan);
            
        }
    }
    else if(strcmp(tokens[0].value,"echo")==0){
        int i=1;
        while(i<tok_count-1 && tokens[i].type!=TOK_COMMA){
            printf("%s ",tokens[i].value);
            i++;
        }
        if(tok_count>2){
            printf("\n");
        }
    }

}