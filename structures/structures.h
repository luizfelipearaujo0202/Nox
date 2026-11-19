#pragma once

#include <stddef.h>
#include <stdio.h>
#include <string.h>
// structures.h

typedef enum {
__NoxBool_FALSE_VALUE,
__NoxBool_TRUE_VALUE
} __NoxValue_BOOLEAN_TYPE;

// ==== DYNAMIC ARRAY ====

// Destructor, destroy a single element
typedef void (__NoxDynaArr_destructor)(void* element);

// Copy, used by dynamic array to copy a @src to @dest
typedef __NoxValue_BOOLEAN_TYPE
  (__NoxDynaArr_copy)(void* dest, const void* src);

// Compare, used by search of dynamic array
typedef __NoxValue_BOOLEAN_TYPE
  (__NoxDynaArr_compare)
  (const void* element, const void* criteria);


  typedef
struct __NoxDynaArr__ {
  void* data;
  size_t count;
  size_t capacity;
  size_t element_size;
  __NoxDynaArr_destructor* destructor;
  __NoxDynaArr_copy* copier;
  __NoxDynaArr_compare* comparator;
} __NoxDynaArr__;

// =======================

// ==== MEMORY LOGGER ====

  typedef
struct {
  __NoxDynaArr__* data;
  int initialized;
} __NoxMem_context;

  typedef 
struct {
  int freed;
  void* ptr;
  int size;
  const char* caller;
  const char* func;
} __NoxMem_record;

// =======================

// ==== LEXER ====

typedef enum
{
NEW_LINE, // '\n'

IDENTIFIER, // variável
LITERAL, // valor literal

PLUS, // '+'
MINUS, // '-'
STAR, // '*'
SLASH, // '/'
PERCENT, // '%'

EQUAL, // '=='
NOT_EQUAL, // '!='
LESS, // '<'
GREATER, // '>'
LESS_EQUAL, // '<='
GREATER_EQUAL, // '>='

ASSIGN, // '='
PLUS_ASSIGN, // '+='
MINUS_ASSIGN, // '-='
STAR_ASSIGN, // '*='
SLASH_ASSIGN, // '/='
PERCENT_ASSIGN, // '%='

SHOW_LINE, // função de mostrar mensagem

LPAREN, // '('
RPAREN, // ')'
LBRACE, // '{'
RBRACE, // '}'
PIPE, // '|'

IF,
ELSE, 
WHILE,
RETURN,

AND,
OR,
NOT,

INVALID_TOKEN,
EOF_TOKEN,
} 
__NoxToken_TYPE;

typedef enum {
VALUE_NONE,
VALUE_INT_LITERAL,
VALUE_FLOAT_LITERAL,
VALUE_STRING_LITERAL,
VALUE_TRUE,
VALUE_FALSE,
VALUE_NULL_LITERAL
} __NoxToken_LITERAL_TYPE;

typedef struct {
  __NoxToken_TYPE _type;
  __NoxToken_LITERAL_TYPE _literal_type;
  const char* start;

  size_t line;
  size_t column;

  size_t length;
} __NoxToken__;

// ===============