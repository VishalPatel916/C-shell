#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include <sys/wait.h>
#include "prompt.h"
#include "input.h"
#include "parser.h"
#include "hop.h"
#include "reveal.h"
#include "log.h"
#include "arbitarycommand.h"
#include "redirection.h"
#include "sequential.h"
#include "activity.h"
#include "keysignal.h"
#include "fgbg.h"

char home_dir[PATH_MAX];
char prev_dir[PATH_MAX];

int main(){
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);
    char **cmd=(char**)malloc(sizeof(char*)*15);
    
    for(int i=0;i<15;i++){
        cmd[i]=(char*)malloc(sizeof(char)*1024);
    }
    int index=0,flag=0,imp_cmd=-1;
    
    if (!getcwd(home_dir, sizeof(home_dir))) {
        perror("getcwd");
        return 1;
    }
    
    char *backgroundName;
    while(1){
        // for(int j=0;j<job_count;j++){
        //     printf("cammand %s %d\n",jobs[j]->command,jobs[j]->state);
        // }
        signal(SIGINT, sigint_handler);
        signal(SIGTSTP, sigtstp_handler);
        char *str;
        if(imp_cmd < 0){
            prompt_print(home_dir);
            // fflush(stdout);
            str=input();
            if (!str) {        
                handle_eof();
            }
        }
        else{
            strcpy(str,cmd[imp_cmd]);
            imp_cmd=-2;
        }
        //llm code start
        pid_t pid;
        int status;
        while ((pid= waitpid(-1, &status, WNOHANG)) > 0) {

            for(int i=0;i<job_count;i++){
                if(jobs[i]->pid==pid){
                    backgroundName=jobs[i]->command;
                    for(int j=i;j<job_count-1;j++){
                        jobs[j]=jobs[j+1];
                    }
                    job_count--;
                    break;
                }
            }
            if (WIFEXITED(status)) {              
                printf("%s with pid %d exited normally\n",backgroundName,pid);
            }
            else{
                printf("%s with pid %d exited abnormally\n",backgroundName,pid);
            }
        }
        // llm code end
        tokenise(str);
        if(!parse_shell_cmd()){
            printf("%s\n","Invalid Syntax!");
            continue;
        }
        
        
        int no_comma=0;int pip=0;
        
        for(int i=0;i<tok_count;i++){
            if(tokens[i].type==TOK_COMMA || tokens[i].type==TOK_AND ){
                no_comma++;
            }
            if(tokens[i].type==TOK_PIPE || tokens[i].type==TOK_OUTPUT || tokens[i].type==TOK_OUTPUT_APPEND || tokens[i].type==TOK_INPUT ){
                pip++;
            }
        }
        if(no_comma>0){
           sequential();
        }
        else if(!pip &&strcmp(tokens[0].value,"hop")==0){
            hop(home_dir,prev_dir,0);
        }
        else if(!pip && strcmp(tokens[0].value,"reveal")==0){
            reveal(home_dir,prev_dir,0);
        }
        else if(!pip && strcmp(tokens[0].value,"log")==0){
            imp_cmd=lo_g(cmd,&index,&flag,0);
        }
        else if(!pip &&strcmp(tokens[0].value,"activities")==0){
            activity();
        }
        else if(!pip &&strcmp(tokens[0].value,"ping")==0){
            ping(0);
        }
        else if(!pip && strcmp(tokens[0].value,"fg")==0){
            fg(0);
        }
        else if(!pip && strcmp(tokens[0].value,"bg")==0){
            bg(0);
        }
        else{
            redirection(0);
        }
        if( strcmp(tokens[0].value,"log")!=0 && imp_cmd!=-2){
            imp_cmd=-1;
            if(index>0){
                if(strcmp(cmd[((index-1)%15)],str)!=0){
                    strcpy(cmd[(index%15)],str);
                    index++;
                    if(index>=15){
                        flag=1;
                    }
                }
            }
            else{
                strcpy(cmd[(index%15)],str);
                index++;
                if(index>=15){
                    flag=1;
                }
            }
        }
    }
   
    
}


// shell_cmd  ->  cmd_group ((& | &&) cmd_group)* &?
// cmd_group ->  atomic (\| atomic)*
// atomic -> name (name | input | output)*
// input -> < name | <name
// output -> > name | >name | >> name | >>name
// name -> r"[^|&><;]+"
