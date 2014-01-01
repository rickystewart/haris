#ifndef __PARSE_H
#define __PARSE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "schema.h"
#include "lex.h"
#include "hash.h"

#define PARSER_MAX_STACK 100

typedef enum {
  PARSE_MEM_ERROR, PARSE_LEX_ERROR, PARSE_UNEXPECTED_TOKEN, 
  PARSE_UNEXPECTED_EOF, PARSE_REDUNDANT_SYMBOL, PARSE_UNDEF_SYMBOL,
  PARSE_INVALID_QUALIFIER, PARSE_INVALID_TYPE, PARSE_IO_ERROR,
  PARSE_STACK_OVERFLOW
} ParserError;

/* 
  PARSING MODUS:
  To use the parser, simply implement the following steps:
  1) First, create the parser. The create_parser() function takes
  no arguments.
  2) Next, bind the parser to an input stream. The bind_parser()
  function takes as its input a parser, a FILE * (the input stream),
  and a char * (the name of the file that we are reading). The char *
  is mainly for the parser's organizational purposes, though you can
  use NULL as the file name if you don't have an adequate name for
  the input stream.
  3) Finally, if both of those operations succeeded, we can call parse()
  to run the parser and generate output.
  4) Repeat steps 2-3 as many times as necessary; a single parser can
  be rebound an arbitrary number of times. If you bind multiple input
  streams to a parser, the compiled schema will reflect all of the input
  streams you've parsed.
  5) Finally, destroy the parser with destroy_parser(). This will destroy
  all of the parser's resources, including the parsed schema. Do not call
  this function until you have no more use of your parsed schema.

  If we call the "parse" function and it succeeds, then the "schema"
   attribute of the structure is the output of the parser, and it can
   be extracted and manipulated manually. If the parse fails, then
   the parser will do its best to report the nature of the failure. The
   type of failure will be stored in ther "errno" field, and the
   error reporting strategy will vary based on the type of error.

   - If the error is a memory error or an EOF error, no further information
   is available.
   - If the error is a lex error, analyze p->lex to get the specific lexer
   err (if there is one).
   - If the error is an unexpected token error, the token was pushed back
   onto the input stream. Call next_token on p->lex to see what the token
   was. 
   - If the error is a redundant symbol, undefined symbol, or invalid type error, 
   check p->errbuf to get the name of the symbol in error.
   - If the error is an invalid qualifier error, check p->errbuf to get the
   name of the type in error.

   In any case, you can check the lexer's line number to find out (roughly)
   where the error occurred.

   In general, there is no way to recover from a parse error. Once a parse
   error has occurred, your only choice is to discover the cause of the error
   and then destroy the parser.
*/
typedef struct {
  ParsedSchema *schema;
  Lexer *lex;
  TypeHash *hash;
  ParserError errno;
  char *errbuf;
  int stack;
} Parser;

Parser *create_parser(void);
void destroy_parser(Parser *);

int bind_parser(Parser *, FILE *, char *);

void diagnose_parse_error(Parser *);

int parse(Parser *);

#endif
