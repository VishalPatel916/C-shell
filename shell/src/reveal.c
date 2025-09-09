#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/limits.h>
#include "reveal.h"
#include <string.h>
#include "parser.h"

int lexico( const struct dirent **a , const struct dirent **b){
    return strcmp((*a)->d_name,(*b)->d_name);
}

void reveal(char *home,char *prev_dir){
    char target[PATH_MAX];
    strcpy(target,home);
    int l=0,a=0;
    char *sym;
    for(int i=1;i<tok_count-1;i++){
        
        sym=tokens[i].value;
        if(sym[0]=='-' && strlen(sym)>1){
            for(int j=1;sym[j];j++){
                if(sym[j]=='a'){
                    a=1;
                }
                else if(sym[j]=='l'){
                    l=1;
                }
            }
        }
        else if(strcmp(sym,"~")==0){
            strcpy(target,home);
        }
        else if(strcmp(sym,"-")==0){
            if(strlen(prev_dir)>0){
                strcpy(target,prev_dir);
            }
        }
        else{
            strcpy(target,sym);
        }

    }
    struct dirent **files;
    int n=scandir(target,&files,NULL,lexico);

    if(n<0){
        printf("No such directory!\n");
        return ;
    }
    char *name;
    for(int i=0;i<n;i++){
        name=files[i]->d_name;
        if(a==1){
            printf("%s ",name);
        }
        else{
            if(name[0]!='.'){
                printf("%s ",name);
            }           
        }
        if(l){
            printf("\n");
        }

    }
}