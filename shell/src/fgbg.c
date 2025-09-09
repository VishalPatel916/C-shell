#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "activity.h"
#include "parser.h"
#include "sequential.h"
void fg(){
    int job_no;
    if(tok_count<=2){
        job_no=job_count-1;
    }
    else{
        job_no=atoi(tokens[1].value);
        job_no--;
    }
    if(job_no>=job_count){
        printf("No such job\n");
        return;
    }
    printf("%s\n",jobs[job_no]->command);

    if(jobs[job_no]->state==Stopped){
        if(kill(jobs[job_no]->pid,SIGCONT)==-1){
    
        }
        jobs[job_no]->state=Running;
    }
    
    rc=jobs[job_no]->pid;
    int status;
    waitpid(rc,&status,WUNTRACED);
    rc=0;
    if(WIFSTOPPED(status)){
        jobs[job_count]->state=Stopped;    
    }
    else if(WIFEXITED(status)){
        for(int i=job_no;i<job_count-1;i++) {
            jobs[i] = jobs[i+1];
        }
        job_count--;
    }
}
void bg(){
    int job_no;
    if(tok_count<=2){
        job_no=job_count-1;
    }
    else{
        job_no=atoi(tokens[1].value);
        job_no--;
    }

    if(job_no>=job_count){
        printf("No such job\n");
        return;
    }
    if(jobs[job_no]->state==Stopped){
        if(kill(jobs[job_no]->pid,SIGCONT)==-1){
    
        }
        jobs[job_no]->state=Running;
        printf("[%d] %s &\n",job_no+1,jobs[job_no]->command);
    }
    else{
        printf("Job already running\n");
       
    }


}