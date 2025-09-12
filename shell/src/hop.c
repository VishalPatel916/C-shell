#include <stdio.h>
#include <string.h>
#include <linux/limits.h>
#include <unistd.h>
#include <stdlib.h>  
#include "parser.h"

void hop(char *home_dir,char * prev_dir,int p) {
    char* sym;
    char* home=home_dir;
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    for (int i = p+1; i < tok_count-1; i++) {
        sym = tokens[i].value;
        
        if (strcmp(sym, "~") == 0) {
    
            if (chdir(home) == 0) {
                strcpy(prev_dir, cwd);
                getcwd(cwd, sizeof(cwd));
            }
        }
        else if (strcmp(sym, "..") == 0) {
            if (chdir("..") == 0) {
                strcpy(prev_dir, cwd);
                getcwd(cwd, sizeof(cwd));
            }
        }
        else if (strcmp(sym, "-") == 0) {
            char temp[PATH_MAX];
            strcpy(temp, cwd);
            if (strlen(prev_dir) > 0 && chdir(prev_dir) == 0) {
                strcpy(prev_dir, temp);
                getcwd(cwd, sizeof(cwd));
            }
        }
        else if (strcmp(sym, ".") == 0) {
           
        }
        else if ((strcmp(sym, ";") == 0)|| (strcmp(sym, "&") == 0) || (strcmp(sym, "|") == 0)|| (strcmp(sym, ">>") == 0) || (strcmp(sym, ">") == 0) || (strcmp(sym, "<") == 0) || (strcmp(sym, ",") == 0)){
           break;
        }
        else { 
            if (chdir(sym) == 0) {
                strcpy(prev_dir, cwd);
                getcwd(cwd, sizeof(cwd));
            } else {
                printf("No such directory!\n");
                return ;
            }
        }
    }
}