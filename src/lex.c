#include "lex.h"

static LexerStatus handle_symbol_token(Lexer *, Token *);
static LexerStatus handle_string_token(Lexer *, Token *);
static void handle_comment(Lexer *);

/* =============================PUBLIC INTERFACE============================= */

Lexer *create_lexer(FILE *stream, char *filename)
{
  Lexer *ret = (Lexer*)malloc(sizeof *ret);
  char *buf = (char*)malloc(BUFFER_SIZE);
  if (filename)
    filename = strdup(filename);
  else
    filename = strdup("(unnamed)");
  if (!ret || !buf || !filename) {
    free(ret);
    free(buf);
    free(filename);
    return NULL;
  }
  ret->filename = filename;
  ret->stream = stream;
  ret->buffer = buf;
  ret->line_no = 1;
  ret->previous = fgetc(stream);
  ret->rewound = 0;
  return ret;
}

void destroy_lexer(Lexer *lex)
{
  free(lex->filename);
  free(lex->buffer);
  free(lex);
  return;
}

/* Pushes a single token back onto the lexer stream. This is primarily useful
   for implementing a kind of lookahead. For example, say the parser receives
   a STRING token from the lexer, but it wants to have another function deal
   with the token. You can call push_token at that point to push the STRING
   token back onto the lexer stream; when next_token is called again, the 
   same token will be returned again. This is an efficient constant-time
   operation.

   Only a single token of pushback is generally supported. The interface is 
   not dissimilar to ungetc from the C standard library. */

void push_token(Lexer *lex, Token tok)
{
  lex->pushback = tok;
  lex->rewound = 1;
  return;
}

LexerStatus next_token(Lexer *lex, Token *tok)
{
  int next_char = lex->previous, tmp;

  if (lex->rewound) {
    lex->rewound = 0;
    *tok = lex->pushback;
    return LEXER_OK;
  }

  while (1) {
    if (next_char == EOF)
      return LEXER_DONE;
    else if (SYMBOL_INIT_CHAR(next_char)) 
      return handle_symbol_token(lex, tok);
    else if (next_char == ',') {
      *tok = TOKEN_COMMA;
      lex->previous = fgetc(lex->stream);
      return LEXER_OK;
    } else if (next_char == '(') {
      *tok = TOKEN_LPAR;
      lex->previous = fgetc(lex->stream);
      return LEXER_OK;
    } else if (next_char == ')') {
      *tok = TOKEN_RPAR;
      lex->previous = fgetc(lex->stream);
      return LEXER_OK;
    } else if (next_char == '?') {
      *tok = TOKEN_NULLABLE;
      lex->previous = fgetc(lex->stream);
      return LEXER_OK;
    } else if (next_char == '[') {
      tmp = fgetc(lex->stream);
      if (tmp != ']') {
        lex->errno = UNEXPECTED_CHAR;
        return LEXER_ERROR;
      }
      *tok = TOKEN_LIST;
      lex->previous = fgetc(lex->stream);
      return LEXER_OK;
    } else if (next_char == '"') 
      return handle_string_token(lex, tok);
    else if (next_char == '#') {
      handle_comment(lex);
      next_char = lex->previous;
      continue;
    } else if (next_char == '@') {
      *tok = TOKEN_FORWARD;
      lex->previous = fgetc(lex->stream);
      return LEXER_OK;
    } else if (next_char == '\n') {
      lex->line_no++;
      next_char = lex->previous = fgetc(lex->stream);
      continue;
    } else if (isspace(next_char)) {
      next_char = lex->previous = fgetc(lex->stream);
      continue;
    } else {
      lex->errno = UNEXPECTED_CHAR;
      return LEXER_ERROR;
    }
  }
}

void diagnose_lexer_error(Lexer *lex)
{
  switch (lex->errno) {
  case LONG_STRING:
    fprintf(stderr, 
"Schema file included a string that was far too long around line %ld.\n\
The maximum string size is %d.\n", lex->line_no, BUFFER_SIZE);
    return;
  case UNEXPECTED_CHAR:
    fprintf(stderr, "There was an unexpected character around line %ld.\n",
            lex->line_no);
    return;
  default:
    return;
  }
}

/* =============================STATIC FUNCTIONS============================= */

static LexerStatus handle_symbol_token(Lexer *lex, Token *tok)
{
  int ch, ind = 0;
  for (ch = lex->previous; SYMBOL_CHAR(ch); ch = fgetc(lex->stream)) {
    if (ind == BUFFER_SIZE - 1) {
      lex->errno = LONG_STRING;
      return LEXER_ERROR;
    }
    lex->buffer[ind++] = (char)ch;
  }
  *tok = TOKEN_SYMBOL;
  lex->buffer[ind] = '\0';
  lex->previous = ch;
  return LEXER_OK;
}

static LexerStatus handle_string_token(Lexer *lex, Token *tok)
{
  int ch, ind = 0;
  for (ch = fgetc(lex->stream); ch != '"'; ch = fgetc(lex->stream)) {
    if (ind == BUFFER_SIZE - 1) {
      lex->errno = LONG_STRING;
      return LEXER_ERROR;
    } else if (ch == EOF) {
      lex->errno = UNEXPECTED_CHAR;
      return LEXER_ERROR;
    } else if (ch == '\\') {
      ch = fgetc(lex->stream);
      if (ch == EOF) {
	      lex->errno = UNEXPECTED_CHAR;
	      return LEXER_ERROR;
      }
    }
    lex->buffer[ind++] = (char)ch;
  }
  *tok = TOKEN_STRING;
  lex->buffer[ind] = '\0';
  lex->previous = fgetc(lex->stream);
  return LEXER_OK;
}

static void handle_comment(Lexer *lex)
{
  int ch = lex->previous;
  while (ch != '\n' && ch != EOF)
    ch = fgetc(lex->stream);
  lex->previous = fgetc(lex->stream);
  lex->line_no++;
  return;
}
