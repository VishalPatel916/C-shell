#include <stdio.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "sequential.h"
#include "activity.h"
#include "parser.h"
#include "keysignal.h"

void sigint_handler(int sig){
    if(rc >0) {
        kill(rc, SIGINT);
    }
    printf("\n");    
}

void sigtstp_handler(int sig){
    if(rc >0){
        
        kill(rc, SIGTSTP); int f=-1;
        for(int i=0;i<job_count;i++){
            if(jobs[i]->pid==rc){
                f=i;
                break;
            }
        }
        if(f==-1){
            jobs[job_count]=malloc(sizeof(job));
            jobs[job_count]->pid = rc;
            jobs[job_count]->command = tokens[0].value;
            jobs[job_count]->state = Stopped;
            job_count++;
            printf("\n[%d] Stopped %s\n", job_count, jobs[job_count-1]->command);
        }
        else{
            jobs[f]->state = Stopped;
            printf("\n[%d] Stopped %s\n", f+1, jobs[f]->command);
        }
        
        rc=0;
        
    }
    else{
        jobs[job_count]=malloc(sizeof(job));
        jobs[job_count]->pid = rc;
        jobs[job_count]->command = tokens[0].value;
        jobs[job_count]->state = Stopped;
        job_count++;
        printf("\n[%d] Stopped %s\n", job_count, jobs[job_count-1]->command);
    }

}

void handle_eof() {
    for (int i = 0; i < job_count; i++) {
       
        kill(jobs[i]->pid, SIGKILL);
    }
    printf("logout\n");
    exit(0);
}
