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
#include "parser.h"
#include "hop.h"
#include "reveal.h"
#include "log.h"
#include "redirection.h"
#include "sequential.h"
#include "activity.h"
#include "fgbg.h"

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
        if(strcmp(tokens[p].value,"hop")==0){
            hop(home_dir,prev_dir,p);
            exit(0);
        }
        else if(strcmp(tokens[p].value,"reveal")==0){
            
            reveal(home_dir,prev_dir,p);
            exit(0);
        }
        else if(strcmp(tokens[p].value,"log")==0){
            int index,flag;
            lo_g(cmd,&index,&flag,p);
            exit(0);
        }
        else if(strcmp(tokens[p].value,"activities")==0){
            activity();
            exit(0);
        }
        else if(strcmp(tokens[p].value,"ping")==0){
            ping(p);
            exit(0);
        }
        else if(strcmp(tokens[p].value,"fg")==0){
            fg(p);
            exit(0);
        }
        else if(strcmp(tokens[p].value,"bg")==0){
            bg(p);
            exit(0);
        }
        else if(execvp(cmd[0],cmd)==-1){
            fprintf(stderr, "%s: Command not found! \n", cmd[0]);
            perror("exec");
            exit(1);
        }
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
    int k=0;int sudotoken[p+n+1];
    sudotoken[i]=p;
    for(int j=p;j<tok_count ;j++){
        if(tokens[j].type==TOK_COMMA || tokens[i].type==TOK_AND){
            cmd[i][k]=NULL;
            k=0;
            break;
        }
        if(tokens[j].type==TOK_PIPE || tokens[j].type==TOK_END){
            cmd[i][k]=NULL;            
            i++;k=0;
            sudotoken[i]=j+1;
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
            int k=0;int fd1,fd2;int and=0;
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
                else if(strcmp(cmd[i][k],"&")==0){
                    and=1;
                    cmd[i][k]=NULL;
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
            if(strcmp(cmd[i][0],"hop")==0){
                hop(home_dir,prev_dir,sudotoken[i]);
                exit(0);
            }
            else if(strcmp(cmd[i][0],"reveal")==0){
                reveal(home_dir,prev_dir,sudotoken[i]);
                exit(0);
            }
            else if(strcmp(cmd[i][0],"log")==0){
                int index,flag;
                lo_g(cmd[i],&index,&flag,sudotoken[i]);
                exit(0);
            }
            else if(strcmp(cmd[i][0],"activities")==0){
                activity();
                exit(0);
            }
            else if(strcmp(cmd[i][0],"ping")==0){
                ping(sudotoken[i]);
                exit(0);
            }
            else if(strcmp(cmd[i][0],"fg")==0){
                fg(sudotoken[i]);
                exit(0);
            }
            else if(strcmp(cmd[i][0],"bg")==0){
                bg(sudotoken[i]);
                exit(0);
            }
            
            else{
                if(and){
                    static int jo=0;
                    jo++;
                    jobs[job_count]=malloc(sizeof(job));
                    jobs[job_count]->command=strdup(tokens[s].value);
                    jobs[job_count]->pid=rc;
                    jobs[job_count++]->state=Running;
                    printf("[%d] %d\n",jo, rc);
                }
                if(execvp(cmd[i][0],cmd[i])==-1){
                    fprintf(stderr, "%s: Command not found! \n", cmd[i][0]);
                    exit(EXIT_FAILURE);
                }
                
            }
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