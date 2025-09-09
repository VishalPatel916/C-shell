#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include<sys/wait.h>
#include "arbitarycommand.h"
#include "parser.h"
#include "redirection.h"
#include <linux/limits.h>

int fg_pid=-1;

void arbtry_cmd(int p){
    char *cmd[tok_count];
    int j=0;int k=0;
    for(int i=p;i<tok_count-1;i++){
        if(tokens[i].type==TOK_COMMA || tokens[i].type==TOK_AND){
            cmd[k]=NULL;
            break;
        }
        if(tokens[i].type==TOK_END){
            cmd[k]=NULL;
        }
        else if(tokens[i].type==TOK_INPUT || tokens[i].type==TOK_OUTPUT || tokens[i].type==TOK_OUTPUT_APPEND || tokens[i].type==TOK_PIPE){
            break;
        }
        else{
            cmd[k]=tokens[i].value;
        }       
        j=k;
        k++;
    }
    cmd[j+1]=NULL;
    fg_pid=fork();
    if(fg_pid<0){
        printf("fork failed");
        return;
    }
    else if(fg_pid==0){
        execvp(cmd[0],cmd);
    }
    else {
        int status;
        waitpid(fg_pid,&status,0);

    }

}
void piping(int p){
    int n=0;
    
    for(int i=p;i<tok_count-1 ;i++){
        if(tokens[i].type==TOK_COMMA || tokens[i].type==TOK_AND){
            break;
        }
        if(tokens[i].type==TOK_PIPE){
            n++;
        }
    }
    int i=0;
    char *cmd[n+1][tok_count];
    int k=0;
    for(int j=p;j<tok_count ;j++){
        if(tokens[j].type==TOK_COMMA || tokens[i].type==TOK_AND){
            cmd[i][k]=NULL;
            k=0;
            break;
        }
        if(tokens[j].type==TOK_PIPE || tokens[j].type==TOK_END){
            cmd[i][k]=NULL;
            i++;k=0;
        }
        else{
            cmd[i][k]=tokens[j].value; 
            k++;   
        }
    }
    
    int connect[n][2];
    for(int i=0;i<n;i++){
        pipe(connect[i]);
    }
    int s=1,t=1;
    for(int i=0;i<=n;i++){
        s=1,t=1;
        int rc=fork();

        if(rc==0){
            int k=0;int fd1,fd2;
            while(cmd[i][k]!=NULL){
                if(strcmp(cmd[i][k],"<")==0){
                    s=0;
                    fd1=open(cmd[i][k+1],O_RDONLY);
                    cmd[i][k]=NULL;
                    dup2(fd1,STDIN_FILENO);
                    close(fd1);                    
                }
                else if(strcmp(cmd[i][k],">")==0){
                    t=0;
                    fd2=open(cmd[i][k+1],O_WRONLY | O_CREAT |O_TRUNC,0644);
                    cmd[i][k]=NULL;
                    dup2(fd2,STDOUT_FILENO);
                    close(fd2);
                }
                else if(strcmp(cmd[i][k],">>")==0){
                     t=0;
                    fd2=open(cmd[i][k+1],O_WRONLY | O_CREAT |O_APPEND,0644);
                    cmd[i][k]=NULL;
                    dup2(fd2,STDOUT_FILENO);
                    close(fd2);
                }
                k++;
            }
            
            if(i>0 && s){
                dup2(connect[i-1][0],STDIN_FILENO);
            }
            if(i<n && t){
                dup2(connect[i][1],STDOUT_FILENO);
            }
           
            for(int j=0;j<n;j++){
                close(connect[j][0]);
                close(connect[j][1]);
            }
            execvp(cmd[i][0],cmd[i]);
            perror("exec");
            exit(1);
        }
    }
    for(int j=0;j<n;j++){
        close(connect[j][0]);
        close(connect[j][1]);
    }
    for(int i=0;i<=n;i++){
        wait(NULL);
    }
}