#include <stdio.h>
#include "activity.h"
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "parser.h"

int job_count=0;
job* jobs[100];
int sort(const void * a,const void * b){
    job *a1=*(job **)a;
    job *b1=*(job **)b;
    return strcmp(a1->command,b1->command);
}
void update(){
    int status;
    int j=0,f;
    for(int i=0;i<job_count;i++){
        f=1;
        int p=waitpid(jobs[i]->pid,&status,WNOHANG | WUNTRACED );
        if(p==0){
            jobs[i-j]->pid=jobs[i]->pid;
            jobs[i-j]->command=jobs[i]->command;
            jobs[i-j]->state=jobs[i]->state;
            continue;
        }
        if(WIFSTOPPED(status)){
            jobs[i]->state=Stopped;
            f=0;
        }
        
        jobs[i-j]->pid=jobs[i]->pid;
        jobs[i-j]->command=jobs[i]->command;
        jobs[i-j]->state=jobs[i]->state;

        if(f && (WIFEXITED(status) || WIFSIGNALED(status))){
            j+=1;
        }
    }
    job_count-=j;
}

void activity(){
    update();
    qsort(jobs,job_count,sizeof(job*),sort);
  
    char *state_str;
    for(int i=0;i<job_count;i++){
        
        if(jobs[i]->state == Running){
            state_str = "Running";
        }
        else if(jobs[i]->state == Stopped){
            state_str = "Stopped";
        }
        else{
            state_str = "Unknown";
        }
        printf("[%d] : %s - %s\n",jobs[i]->pid,jobs[i]->command,state_str);
    }
}

void ping(int p){
    if(tok_count>3+p){
        int pid=atoi(tokens[1+p].value);
        int signal_number=atoi(tokens[2+p].value);
        int actual_signal = signal_number % 32;
        if(kill(pid,actual_signal)==-1){
            if (errno == ESRCH) {
                printf("No such process found\n");
            } 
        }
        else{
            printf("Sent signal %d to process with pid %d\n",actual_signal,pid);
            if(actual_signal==18){
                for(int i=0;i<job_count;i++){
                    if(jobs[i]->pid==pid){
                        jobs[i]->state=Running;
                        break;
                    }
                }
            }
            else if(actual_signal==18){
                for(int i=0;i<job_count;i++){
                    if(jobs[i]->pid==pid){
                        jobs[i]->state=Stopped;
                        break;
                    }
                }
            }
        }
    }
}