#ifndef PARSER_H

#define PARSER_H

typedef enum{
    TOK_NAME,TOK_PIPE,TOK_AND,
    TOK_COMMA,TOK_INPUT,TOK_OUTPUT,
    TOK_OUTPUT_APPEND,TOK_END
}Tokentype;

typedef struct{
    Tokentype type;
    char* value;
}Token;


void tokenise(const char* input);

int parse_shell_cmd();

extern Token tokens[];
extern int tok_count;
extern int current;

#endif