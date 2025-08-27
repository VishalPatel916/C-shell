#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "prompt.h"
#include "input.h"
#include "parser.h"
#include "hop.h"
#include "reveal.h"
#include "log.h"
#include "arbitarycommand.h"

int main(){
    char **cmd=(char**)malloc(sizeof(char*)*15);
    for(int i=0;i<15;i++){
        cmd[i]=(char*)malloc(sizeof(char)*1024);
    }
    int index=0,flag=0,imp_cmd=-1;
    char home_dir[PATH_MAX];
    char prev_dir[PATH_MAX];
    if (!getcwd(home_dir, sizeof(home_dir))) {
        perror("getcwd");
        return 1;
    }

    while(1){
        
        char *str;
        if(imp_cmd==-1){
            prompt_print(home_dir);
            str=input();
        }
        else{
            strcpy(str,cmd[imp_cmd]);
            imp_cmd=-1;
        }
        tokenise(str);
        if(!parse_shell_cmd()){
            printf("%s\n","Invalid Syntax!");
            continue;
        }
        // printf("%d\n",tok_count);
        // for(int i=0;i<tok_count;i++){
        //     printf("%s\n",tokens[i].value);
        // }
        if(strcmp(tokens[0].value,"hop")==0){
            hop(home_dir,prev_dir);
        }
        else if(strcmp(tokens[0].value,"reveal")==0){
            reveal(home_dir,prev_dir);
        }
        else if(strcmp(tokens[0].value,"log")==0){
            imp_cmd=lo_g(cmd,&index,&flag);
        }
        else{
            arbtry_cmd();
        }
        if(strcmp(tokens[0].value,"log")!=0){
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
