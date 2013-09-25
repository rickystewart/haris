#include "parse.h"

static Parser *create_parser_from_schema_hash(FILE *, char *, ParsedSchema *, 
                                              TypeHash *);
static void destroy_parser_but_not_schema_hash(Parser *);

static int assert_lexer_ok(Parser *, LexerStatus);
static int trigger_parse_error(Parser *, ParserError, char *);

static int toplevel_parse(Parser *);

static int parse_struct_def(Parser *);
static int parse_enum_def(Parser *);
static int handle_include(Parser *);
static int parse_extension(Parser *);
static int parse_forward(Parser *);

static ParsedStruct *new_hashed_struct(Parser *, char *);
static ParsedEnum *new_hashed_enum(Parser *, char *);

static int parse_forward_struct(Parser *);
static int parse_forward_enum(Parser *);

static int parse_extend_struct(Parser *);
static int parse_extend_enum(Parser *);

static int parse_structure_elements(Parser *, ParsedStruct *);
static int parse_enum_elements(Parser *, ParsedEnum *);

static int parse_structure_element(Parser *, ParsedStruct *);
static int parse_type_qualifiers(Parser *, int *, int *);

static int handle_field_addition(Parser *, ParsedStruct *, char *, 
                                 TypeHashBucket *, int, int);

static int include_file(Parser *, char *);

static int expect_token(Parser *, Token);
static int unexpected_token_error(Parser *, Token);

/* =============================PUBLIC INTERFACE============================= */

Parser *create_parser(FILE *stream, char *filename)
{
  Parser *parser;
  ParsedSchema *schema = create_parsed_schema();
  TypeHash *hash = create_typehash();
  if (!schema || !hash) goto MemoryAllocationError;
  parser = create_parser_from_schema_hash(stream, filename, schema, hash);
  if (!parser) goto MemoryAllocationError;
  return parser;
 MemoryAllocationError:
  if (schema) destroy_parsed_schema(schema);
  if (hash) destroy_typehash(hash);
  return NULL;
}

/* Destroys the given parser and all of its attribute objects, INCLUDING
   the schema and typehash. */
void destroy_parser(Parser *p)
{
  destroy_parsed_schema(p->schema);
  destroy_typehash(p->hash);
  destroy_parser_but_not_schema_hash(p);
  return;
}

/* Rebinds the given parser to a new stream. This function is useful primarily
   for retaining the schema information of a previous parse and adding new
   data onto the previous schema definition. If you do not wish to retain
   the schema information from a previous parse, destroy the previous parser
   and create a new one rather than rebinding an existing one. 
*/
int rebind_parser(Parser *p, FILE *stream, char *filename)
{
  Lexer *lex = create_lexer(stream, filename);
  if (!lex) return 0;
  ret->lex = lex;
  return 1;
}

/* parse: Run the given parser to completion, handling any errors that
   may occur along the way. */
int parse(Parser *p)
{
  if (p->stack >= PARSER_MAX_STACK) 
    return trigger_parse_error(p, PARSE_STACK_OVERFLOW, NULL);
  return toplevel_parse(p);
}

/* =============================STATIC FUNCTIONS============================= */

static Parser *create_parser_from_schema_hash(FILE *stream, char *filename,
                                              ParsedSchema *schema,
                                              TypeHash *hash)
{
  Lexer *lex = create_lexer(stream, filename);
  Parser *ret = (Parser*)malloc(sizeof *ret);
  if (!lex || !ret) {
    if (lex) destroy_lexer(lex);
    free(ret);
    return NULL;
  }
  ret->lex = lex;
  ret->schema = schema;
  ret->hash = hash;
  ret->errbuf = NULL;
  ret->stack = 0;
  return ret;
}

/* Destroys the given parser and its lexer. Does not destroy the schema
   or typehash associated with the parser, so they can be reused later in
   another parser.
*/
static void destroy_parser_but_not_schema_hash(Parser *p)
{
  destroy_lexer(p->lex);
  if (p->errbuf) free(p->errbuf);
  free(p);
  return;
}

/* Returns 1 if the lexer returned a status of "OK", and 0 otherwise. If 
   the lexer encountered an error or the end of the stream, it also sets the 
   error code in the parser appropriately. This function is appropriate for
   use in any non-top-level parsing helper function, as an EOF is never
   appropriate when we're not at top level.
*/
static int assert_lexer_ok(Parser *p, LexerStatus status)
{
  switch (status) {
  case LEXER_DONE:
    return trigger_parse_error(p, PARSE_UNEXPECTED_EOF, NULL);
  case LEXER_ERROR:
    return trigger_parse_error(p, PARSE_LEX_ERROR, NULL);
  case LEXER_OK:
    return 1;
  default:
    return 0;
  }
}

/* This parse function immediately triggers a parse error. The error code will
   be stored in the errno field of the parser, and the msg parameter (if
   it is not NULL) will be copied into the error buffer. This function 
   always returns 0 (since it always triggers an error).

   The msg parameter will be copied over to the heap. If the copy fails, the
   error will be silently converted into a memory error. (This is to ensure
   that particular error codes will always have access to the error buffer if 
   it is expected.)
*/
static int trigger_parse_error(Parser *p, ParserError err, char *msg)
{
  if (msg) {
    p->errbuf = strdup(msg);
    if (!p->errbuf) p->errno = PARSE_MEM_ERROR;
    else p->errno = err;
  } else {
    p->errno = err;
    p->errbuf = NULL;
  }
  return 0;
}

/* This function handles parsing at the "top level"; that is, the parser
   will be in this function when it's not in the middle of parsing a 
   definition. This function will parse the entire file in order and will
   return once the end of the input stream has been reached. 

   Legal incoming tokens:
   - @, which indicates we should move to the forward declaration handler
   - Any of the symbols "enum", "struct", "include", and "extend", which
   indicates we should move to their respective handlers. Any other symbol
   is not valid.
   - EOF, which indicates that the parse has finished.

   Any other token is a parse error.
*/
static int toplevel_parse(Parser *p)
{
  int result;
  LexerStatus status;
  Token tok;
  while (1) {
    status = next_token(p->lex, &tok);
    switch (status) {
    case LEXER_DONE:
      return 1;
    case LEXER_ERROR:
      p->errno = PARSE_LEX_ERROR;
      return 0;
    case LEXER_OK:
      break;
    }
    switch (tok) {
    case TOKEN_SYMBOL:
      if (!strcmp(p->lex->buffer, "struct"))
        result = parse_struct_def(p);
      else if (!strcmp(p->lex->buffer, "enum"))
        result = parse_enum_def(p);
      else if (!strcmp(p->lex->buffer, "include"))
        result = handle_include(p);
      else if (!strcmp(p->lex->buffer, "extend"))
        result = parse_extension(p);
      else result = unexpected_token_error(p, tok);
      break;
    case TOKEN_FORWARD:
      result = parse_forward(p);
      break;
    default:
      result = unexpected_token_error(p, tok);
    }
    if (!result) return 0;
  }
}

/* This function handles parsing AFTER we have seen a struct keyword at top 
   level. The function will return after the closing parenthesis is 
   encountered, leaving us at the top level once more.
*/
static int parse_struct_def(Parser *p)
{
  ParsedStruct *strct;
  /* Get structure name */
  if (!expect_token(p, TOKEN_SYMBOL)) return 0;
  if (get_type(p->hash, p->lex->buffer)) {
    return trigger_parse_error(p, PARSE_REDUNDANT_SYMBOL, p->lex->buffer);
  } else {
    strct = new_hashed_struct(p, p->lex->buffer);
    if (!strct) return 0;
  }

  return parse_structure_elements(p, strct);
}

/* This function handles parsing AFTER we have seen an enum keyword at top 
   level. The function will return after the closing parenthesis is 
   encountered, leaving us at the top level once more.
*/
static int parse_enum_def(Parser *p)
{
  ParsedEnum *enm;
  /* Get enumeration name */
  if (!expect_token(p, TOKEN_SYMBOL)) return 0;
  if (get_type(p->hash, p->lex->buffer)) {
    return trigger_parse_error(p, PARSE_REDUNDANT_SYMBOL, p->lex->buffer);
  } else {
    enm = new_hashed_enum(p, p->lex->buffer);
    if (!enm) return 0;
  }

  return parse_enum_elements(p, enm);
}

/* This function handles parsing AFTER we have seen an include keyword at top 
   level. The function will return after the closing parenthesis is 
   encountered, leaving us at the top level once more.
*/
static int handle_include(Parser *p)
{
  Token tok;
  if (!expect_token(p, TOKEN_LPAR)) return 0;

  /* Comma-separated list of elements */
  while (1) {
    if (!expect_token(p, TOKEN_STRING)) return 0;
    if (!include_file(p, p->lex->buffer)) return 0;
    /* Either a comma or a right parenthesis will follow */
    if (!assert_lexer_ok(p, next_token(p->lex, &tok))) return 0;
    if (tok == TOKEN_RPAR) return 1;
    else if (tok != TOKEN_COMMA) return unexpected_token_error(p, tok);
    else continue;
  }
}

/* This function handles parsing AFTER we have seen an extend keyword at top
   level. The function will return after the structure or enum has been
   extended, leaving us at the top level once more. 

   What follows is a normal structure or enumeration definition.
*/
static int parse_extension(Parser *p)
{
  if (!expect_token(p, TOKEN_SYMBOL)) return 0;
  if (!strcmp(p->lex->buffer, "struct")) {
    return parse_extend_struct(p);
  } else if (!strcmp(p->lex->buffer, "enum")) {
    return parse_extend_enum(p);
  } else return unexpected_token_error(p, TOKEN_SYMBOL);
}

/* This function handles parsing AFTER we have seen a forward ("@") token
   at top level. The function will return after the structure or enum
   has been declared, leaving us at the top level once more. 
*/
static int parse_forward(Parser *p)
{
  if (!expect_token(p, TOKEN_SYMBOL)) return 0;
  if (!strcmp(p->lex->buffer, "struct")) {
    return parse_forward_struct(p);
  } else if (!strcmp(p->lex->buffer, "enum")) {
    return parse_forward_enum(p);
  } else return unexpected_token_error(p, TOKEN_SYMBOL);
}

/* Create a new structure with the given name, add it to the type hash,
   and return the structure. Returns and sets the parser's error code if an
   error occurs.
*/
static ParsedStruct *new_hashed_struct(Parser *p, char *name)
{
  ParsedStruct *strct = new_struct(p->schema, p->lex->buffer);
  if (!strct) {
    (void)trigger_parse_error(p, PARSE_MEM_ERROR, NULL);
    return NULL;
  } 
  if (!add_struct_to_hash(p->hash, strct->name, strct)) {
    (void)trigger_parse_error(p, PARSE_MEM_ERROR, NULL);
    return NULL;
  } 
  return strct;
}

/* Create a new structure with the given name, add it to the type hash,
   and return the structure. Returns and sets the parser's error code if an
   error occurs.
*/
static ParsedEnum *new_hashed_enum(Parser *p, char *name)
{
  ParsedEnum *enm = new_enum(p->schema, p->lex->buffer);
  if (!enm) {
    (void)trigger_parse_error(p, PARSE_MEM_ERROR, NULL);
    return NULL;
  }
  if (!add_enum_to_hash(p->hash, enm->name, enm)) {
    (void)trigger_parse_error(p, PARSE_MEM_ERROR, NULL);
    return NULL;
  }
  return enm;
}

/* Parses a forward structure declaration AFTER we have encountered the 
   "@" symbol and the "struct". Consumes the structure name from the input 
   and assures the structure exists. 
*/
static int parse_forward_struct(Parser *p)
{
  TypeHashBucket *bucket;
  ParsedStruct *strct;
  if (!expect_token(p, TOKEN_SYMBOL)) return 0;
  bucket = get_type(p->hash, p->lex->buffer);
  if (bucket) {
    if (bucket->tu.tag != TYPE_STRUCT)
      return trigger_parse_error(p, PARSE_INVALID_TYPE, p->lex->buffer);
    else return 1;
  } else {
    strct = new_hashed_struct(p, p->lex->buffer);
    if (!strct) return 0;
    else return 1;
  }
}

/* Parses a forward enum declaration AFTER we have encountered the 
   "@" symbol and the "enum". Consumes the structure name from the input 
   and assures the enum exists. 
*/
static int parse_forward_enum(Parser *p)
{
  TypeHashBucket *bucket;
  ParsedEnum *enm;
  if (!expect_token(p, TOKEN_SYMBOL)) return 0;
  bucket = get_type(p->hash, p->lex->buffer);
  if (bucket) {
    if (bucket->tu.tag != TYPE_ENUM)
      return trigger_parse_error(p, PARSE_INVALID_TYPE, p->lex->buffer);
    else return 1;
  } else {
    enm = new_hashed_enum(p, p->lex->buffer);
    if (!enm) return 0;
    else return 1;
  }
}

/* This function handles parsing after we have seen an "extend" token and a
   "struct" token. 
*/
static int parse_extend_struct(Parser *p)
{
  TypeHashBucket *bucket;
  if (!expect_token(p, TOKEN_SYMBOL)) return 0;
  bucket = get_type(p->hash, p->lex->buffer);
  if (!bucket) 
    return trigger_parse_error(p, PARSE_UNDEF_SYMBOL, p->lex->buffer);
  if (bucket->tu.tag != TYPE_STRUCT)
    return trigger_parse_error(p, PARSE_INVALID_TYPE, p->lex->buffer);
  return parse_structure_elements(p, bucket->tu.type.strct);
}

/* This function handles parsing after we have seen an "extend" token and a
   "enum" token. 
*/
static int parse_extend_enum(Parser *p)
{
  TypeHashBucket *bucket;
  if (!expect_token(p, TOKEN_SYMBOL)) return 0;
  bucket = get_type(p->hash, p->lex->buffer);
  if (!bucket) 
    return trigger_parse_error(p, PARSE_UNDEF_SYMBOL, p->lex->buffer);
  if (bucket->tu.tag != TYPE_ENUM)
    return trigger_parse_error(p, PARSE_INVALID_TYPE, p->lex->buffer);
  return parse_enum_elements(p, bucket->tu.type.enm);
}

/* This function parses the elements of a structure after we have determined
which structure it is (that is, after we've gotten its name). The opening
parenthesis should be waiting in the stream when this function is called
and the function returns once we encounter the closing parenthesis. 
*/
static int parse_structure_elements(Parser *p, ParsedStruct *strct)
{
  Token tok;

  if (!expect_token(p, TOKEN_LPAR)) return 0;

  /* Comma-separated list of elements */
  while (1) {
    if (!parse_structure_element(p, strct)) return 0;
    /* Either a comma or a right parenthesis will follow */
    if (!assert_lexer_ok(p, next_token(p->lex, &tok))) return 0;
    if (tok == TOKEN_RPAR) return 1;
    else if (tok != TOKEN_COMMA) return unexpected_token_error(p, tok);
    else continue;
  }

}

/* As above, this function is called after we've encountered the enumeration
   name but before the left parenthesis. This function returns once we've 
   found the right parenthesis. 
*/
static int parse_enum_elements(Parser *p, ParsedEnum *enm)
{
  Token tok;

  if (!expect_token(p, TOKEN_LPAR)) return 0;

  /* Comma-separated list of elements */
  while (1) {
    if (!expect_token(p, TOKEN_SYMBOL)) return 0;
    if (enum_name_collide(enm, p->lex->buffer)) 
      return trigger_parse_error(p, PARSE_REDUNDANT_SYMBOL, p->lex->buffer);
    if (!add_enumerated_value(enm, p->lex->buffer)) return 0;
    /* Either a comma or a right parenthesis will follow */
    if (!assert_lexer_ok(p, next_token(p->lex, &tok))) return 0;
    if (tok == TOKEN_RPAR) return 1;
    else if (tok != TOKEN_COMMA) return unexpected_token_error(p, tok);
    else continue;
  }
}

/* This function does the dirty work of parsing a single structure element
within a structure definition. For example, in the definition
  struct Foo ( Int32 baz = 100 )
... this function will be called after the opening parenthesis has been 
consumed from the stream, but before the structure element. This function 
parses a single structure element, leaving whatever tokens immediately 
follow it (in this case a right parenthesis) on the stream. 

The form of a structure element is
    TYPE name 
*/
static int parse_structure_element(Parser *p, ParsedStruct *strct)
{
  int list, nullable;
  TypeHashBucket *type;
  /* Get type from type name */
  if (!expect_token(p, TOKEN_SYMBOL)) return 0;
  type = get_type(p->hash, p->lex->buffer);
  if (!type) 
    return trigger_parse_error(p, PARSE_UNDEF_SYMBOL, p->lex->buffer);
  /* Process type qualifiers. */
  if (!parse_type_qualifiers(p, &list, &nullable)) return 0;
  /* Process field name */
  if (!expect_token(p, TOKEN_SYMBOL)) return 0;
  return handle_field_addition(p, strct, p->lex->buffer, type, list, nullable);
}

/* Process type qualifiers. To do this, we consume one token. If it's a
     ? token, then the given type is not a list type (because the list
     token has to come first) and we can move onto the next section. If
     it's a list token, we have to check whether the list is nullable
     by consuming another token. Otherwise, we have to push the token
     back onto the stream in preparation for the next section.

     "list" and "nullable" are boolean out parameters. If the given type is
     a list, *list will be set to 1, and if the given type is nullable,
     *nullable will be set to 1. The input stream will be set to the token
     after the type qualifiers.
*/
static int parse_type_qualifiers(Parser *p, int *list, int *nullable)
{
  Token tok;
  *list = 0;
  *nullable = 0;
  if (!assert_lexer_ok(p, next_token(p->lex, &tok))) return 0;
  switch (tok) {
  case TOKEN_LIST:
    *list = 1;
    if (!assert_lexer_ok(p, next_token(p->lex, &tok))) return 0;
    if (tok == TOKEN_NULLABLE) *nullable = 1;
    else push_token(p->lex, tok);
    break;
  case TOKEN_NULLABLE:
    *nullable = 1;
    break;
  default:
    push_token(p->lex, tok);
  }
  return 1;
}

static int handle_field_addition(Parser *p, ParsedStruct *strct, char *name, 
                                 TypeHashBucket *bucket, int list, int nullable)
{
  if (struct_name_collide(strct, name)) 
    return trigger_parse_error(p, PARSE_REDUNDANT_SYMBOL, name);
  switch (bucket->tu.tag) {
  case TYPE_SCALAR_BUILTIN:
    if (list) {
      if (!add_list_of_scalars_field(strct, name, nullable, 
                                     bucket->tu.type.scalar_builtin))
        goto MemError;
    } else if (nullable) {
      goto InvalidQualifier;
    } else {
      if (!add_scalar_field(strct, name, bucket->tu.type.scalar_builtin))
        goto MemError;
    }
    break;
  case TYPE_ENUM:
    if (list) {
      if (!add_list_of_enums_field(strct, name, nullable, bucket->tu.type.enm))
        goto MemError;
    } else if (nullable) {
      goto InvalidQualifier;
    } else {
      if (!add_enum_field(strct, name, bucket->tu.type.enm))
        goto MemError;
    }
    break;
  case TYPE_STRUCT:
    if (list) {
      if (!add_list_of_structs_field(strct, name, nullable, 
                                     bucket->tu.type.strct))
        goto MemError;
    } else {
      if (!add_struct_field(strct, name, nullable, bucket->tu.type.strct))
        goto MemError;
    }
    break;
  case TYPE_TEXT:
    if (list) goto InvalidQualifier;
    if (!add_text_field(strct, name, nullable)) goto MemError;
  }
  return 1;
 InvalidQualifier:
  return trigger_parse_error(p, PARSE_INVALID_QUALIFIER, strct->name);
 MemError:
  return trigger_parse_error(p, PARSE_MEM_ERROR, NULL);
}

/* Open up a new file and parse it, merging the results of the parse
   with the given parser. */
static int include_file(Parser *p, char *filename)
{
  Parser *included;
  int ret;
  FILE *stream = fopen(filename, "r");
  if (!stream) return trigger_parse_error(p, PARSE_IO_ERROR, filename);
  included = create_parser_from_schema_hash(stream, filename, p->schema, 
                                            p->hash);
  if (!included) return trigger_parse_error(p, PARSE_MEM_ERROR, NULL);
  included->stack = p->stack + 1;
  ret = parse(included);
  if (!ret) {
    (void)trigger_parse_error(p, included->errno, included->errbuf);
  }
  destroy_parser_but_not_schema_hash(included);
  return ret;
}

/* Unconditionally expect the given token, raising an unexpected token
   error if the token doesn't match. If the match succeeds, then we return
   1, and you will be able to access the lexer's buffers to find the value
   of the token just as if you had retrieved the token manually. 
*/
static int expect_token(Parser *p, Token expected)
{
  Token tok;
  if (!assert_lexer_ok(p, next_token(p->lex, &tok))) return 0;
  if (tok != expected) return unexpected_token_error(p, tok);
  return 1;
}

/* Raise an unexpected token error. This is a special case of error raising
   where we must ensure the token goes back into the input stream; this
   function will take care of that for us.
*/
static int unexpected_token_error(Parser *p, Token tok)
{
  push_token(p->lex, tok);
  return trigger_parse_error(p, PARSE_UNEXPECTED_TOKEN, NULL);
}
