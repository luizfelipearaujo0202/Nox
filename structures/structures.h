#pragma once

#include <stddef.h>
#include <stdio.h>
#include <string.h>
// structures.h

typedef enum {
__NoxBool_FALSE_VALUE,
__NoxBool_TRUE_VALUE
} __NoxValue_BOOLEAN_TYPE;

// ==== DEBUG ====
typedef enum {
  DEBUG_NONE = 0,
  DEBUG_VERIFY = 1 << 0, // 0001
  DEBUG_INFO = 1 << 1, // 0010
  DEBUG_ERROR = 1 << 2, // 0100
  DEBUG_ALL = 0xFFFF // 1111
} __NoxDebug_LEVEL;

extern unsigned int _NOX_DEBUG_ENABLED;

#define _NOX_DEBUG_LOG(level, module, function, ...) \
  do { \
    if ((_NOX_DEBUG_ENABLED & (level)) != 0) { \
      const char* _lvl_str = \
        ((level) & DEBUG_VERIFY) ? "verify" : \
        ((level) & DEBUG_INFO) ? "info" : \
        ((level) & DEBUG_ERROR) ? "error" : "unknown"; \
      \
      if ((function) != NULL && (function)[0] != '\0') { \
        printf("[%s - %s (%s)] ", (module), (function), _lvl_str); \
      } \
      else if ((module) != NULL && (module)[0] != '\0') { \
        printf("[%s (%s)] ", (module), _lvl_str); \
      } \
      printf(__VA_ARGS__); \
    } \
  } while (0)
// ===============

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
struct {
  void* data;
  size_t count;
  size_t capacity;
  size_t element_size;
  __NoxDynaArr_destructor* destructor;
  __NoxDynaArr_copy* copier;
  __NoxDynaArr_compare* comparator;
} __NoxDynaArr__;

// =======================

// ==== LEXER USE ====

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

SHOW_LINE, //

LPAREN, // '('
RPAREN, // ')'
LBRACE, // '{'
RBRACE, // '}'

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

// ===================