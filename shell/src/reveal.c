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

int lexico_ls(const struct dirent **a, const struct dirent **b) {
    return strcoll((*a)->d_name, (*b)->d_name);
}

void reveal(char *home,char *prev_dir,int p) {
    char target[PATH_MAX];
    getcwd(target, sizeof(target));
    int l=0,a=0;
    char *sym;int c=0;
    for(int i=p+1;i<tok_count-1;i++){
        
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
            else{
                printf("No such directory!\n");
                return ;
            }
        }
        else if ((strcmp(sym, ";") == 0)|| (strcmp(sym, "&") == 0) || (strcmp(sym, "|") == 0) || (strcmp(sym,"<")==0)||(strcmp(sym,">")==0) || (strcmp(sym,">>")==0) ){
           break;
        }
        else{
            strcpy(target,sym);
            c++;
        }

    }
    if(c>1){
        printf("reveal: Invalid Syntax!\n");
        return ;
    }
    struct dirent **files;
    
    int n;
    if(l || a){
        n=scandir(target,&files,NULL,lexico);
    }
    else{
        n=scandir(target,&files,NULL,lexico_ls);
    }   
    if(n<0){
        printf("No such directory! \n");
        return;
    }
    char *name;
    int i=0;
    if(l&& (!a)){i=2;}
    for(;i<n;i++){
        
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
    if(!l){
        printf("\n");
    }
}