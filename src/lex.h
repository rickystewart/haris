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

/* LEXER MODUS:
   1) Create a lexer with create_lexer(). This function takes as input
   an input stream to analyze and the name of the file to be parsed
   (which is used only to generate error messages). The name of the 
   file is not used by the library in any other way. If you do not have
   an adequate file name to pass to create_lexer(), use NULL.
   2) Call next_token() repeatedly on the lexer. The lexer status is
   the return value of the function; if the lexer successfully returned
   another token, it will return LEXER_OK. If the lexer reached the
   end of the input stream, it will return LEXER_DONE. If the lexer
   encountered an error, it will return LEXER_RROR, and the error code
   will be stored in the `errno` field of the lexer. 

   If a token was successfully returned, it will be stored in the
   Token * out parameter.
   3) When you are done pulling tokens from the input stream, call
   destroy_lexer() to get rid of the lexer. This will not close the
   input file for you. In general, you cannot use the input file in
   any meaningful way in between token retrievals; once you bind an
   input stream to a lexer, you shouldn't pull characters from the
   stream (except through the lexer).

   The other notable lexing function is push_token(), which pushes a
   token onto the lexer stream. If you push a token onto the lexer
   stream, it will be pulled the next time you call next_token()
   (for example, if you call push_token(lex, TOKEN_LPAR);, TOKEN_LPAR
   will be the thing you get when you call next_token() the next
   time, regardless of what is in the file). This is an efficient
   operation. Pushing a token onto the lexer stream does not
   affect what is in the file; lexing will resume at the current position
   in the file after you pull the pushed token. You can only push a single
   token onto the lexer stream; if you push more, you will only be
   able to pull the most recently pushed token.
*/

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
