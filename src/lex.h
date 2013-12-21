#ifndef __LEX_H
#define __LEX_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define BUFFER_SIZE 256

#define SYMBOL_INIT_CHAR(ch) (isalpha(ch))
#define SYMBOL_CHAR(ch) (isalpha(ch) ||                    \
                         (ch == '_') ||                    \
                         (ch >= '0' && ch <= '9'))

typedef enum { 
  TOKEN_LPAR, TOKEN_RPAR, TOKEN_FORWARD, TOKEN_LIST, TOKEN_NULLABLE, 
  TOKEN_COMMA, TOKEN_STRING, TOKEN_SYMBOL
} Token;

typedef enum {
  LEXER_OK, LEXER_DONE, LEXER_ERROR
} LexerStatus;

typedef enum {
  LONG_STRING, UNEXPECTED_CHAR
} LexerError;

typedef struct {
  char *filename;
  FILE *stream;
  char *buffer;
  long line_no;
  int previous; 
  LexerError errno;
  int rewound;
  Token pushback;
} Lexer;

Lexer *create_lexer(FILE *, char *);
void destroy_lexer(Lexer *);

LexerStatus next_token(Lexer *, Token *);
void push_token(Lexer *, Token);

#endif
