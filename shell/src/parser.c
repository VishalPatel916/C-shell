#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "parser.h"

#define MAX 100

Token tokens[MAX];

int current=0;
int tok_count=0;

void add_token(Tokentype type,char *value){
    tokens[tok_count].type=type;
    tokens[tok_count].value=strdup(value);
    tok_count++;
}

void tokenise(const char* input){
    tok_count = 0; current = 0; 
    int i=0;
    while(input[i]){

        if(isspace(input[i]) || input[i]=='"' ){
            i++;
            continue;
        }

        if(input[i]=='|'){
            add_token(TOK_PIPE,"|");
            i++;
        }
        else if(input[i]=='&'){
            add_token(TOK_AND,"&");
            i++;
            
        }
        else if(input[i]==';'){
            add_token(TOK_COMMA,";");
            i++;
            
        }
        else if(input[i]=='<'){           
            add_token(TOK_INPUT,"<");
            i++;            
        }
        else if(input[i]=='>'){
            if(input[i+1] && input[i+1]=='>'){
                add_token(TOK_OUTPUT_APPEND,">>");
                i+=2;
            }
            else{
                add_token(TOK_OUTPUT,">");
                i++;
            }                      
        }
        else{
            char buffer[256];
            int j=0;
            while(input[i] && !(isspace(input[i]))&& input[i]!='>' && input[i]!='<' && input[i]!='|' && input[i]!='&' && input[i]!=';'&& input[i]!='"' ){
                buffer[j]=input[i];
                i++;j++;
            }
            buffer[j]='\0';
            add_token(TOK_NAME,buffer);
            
        }
    }
    add_token(TOK_END,"");
}

Token *peek(){
    return &tokens[current];
}
Token *skip(){
    return &tokens[current++];
}

int check_name(char *value){
    char *special="^|&><;";
    for(int i=0;value[i]!='\0';i++){
        if(strchr(special,value[i])!=NULL){
            return 0;
        }
    }
    return 1;
}
int parser_atomic(){
    if(peek()->type!=TOK_NAME){
        return 0;
    }
    else{
        if(check_name(peek()->value)==0){
            return 0;
        }
    }
    skip();
    while(peek()->type==TOK_NAME || peek()->type==TOK_INPUT || peek()->type==TOK_OUTPUT || peek()->type==TOK_OUTPUT_APPEND){
        int f=1;
        if (peek()->type==TOK_NAME) {
            if(check_name(peek()->value)==0){
                return 0;
            }
            f=0;
        }   
        skip();
        if (f && peek()->type!=TOK_NAME) {
            return 0;          
        }  

    }    

    return 1;
}

int parse_cmd_group(){
    if(parser_atomic()==0){
        return 0;
    }
    while(peek()->type==TOK_PIPE){ 

        skip();
        if(parser_atomic()==0){
            return 0;
        }
    }    

    return 1;

}

int parse_shell_cmd(){

    if(parse_cmd_group()==0){
        return 0;
    }
    while(peek()->type==TOK_AND || peek()->type==TOK_COMMA){

        if(peek()->type==TOK_AND){
            skip();
            if(peek()->type==TOK_END){
                return 1;
            }
            if(parse_cmd_group()==0){
                return 0;
            }
        }
        else{
            skip();
            if(parse_cmd_group()==0){
                return 0;
            }
        }   

    }
    
    if(peek()->type!=TOK_END){
        return 0;
    }

    return 1;
}