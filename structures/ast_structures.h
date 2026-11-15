#pragma once

// structures/ast_structures.h

#include <stddef.h>
#include "structures.h"
#include "../parser/var/var.h"
#include "../error/error_structures.h"

typedef enum {
AST_LITERAL,
AST_IDENTIFIER,
AST_BINARY,
AST_UNARY,
AST_ASSIGNMENT,

AST_IF
} __NoxAST_TYPE;

typedef enum {
AST_PLUS,
AST_MINUS,
AST_MULTIPLY,
AST_DIVIDE,
AST_MODULE,

AST_EQUAL,        // ==
AST_NOT_EQUAL,    // !=
AST_LESS,         // <
AST_GREATER,      // >
AST_LESS_EQUAL,   // <=
AST_GREATER_EQUAL // >=
} __NoxAST_OPERATOR;


  typedef 
struct __NoxAST_NODE {
  __NoxAST_TYPE _type;

  union {
    __NoxToken__* token;

    struct {
      struct __NoxAST_NODE* left;
      __NoxAST_OPERATOR op;
      struct __NoxAST_NODE* right;
    } binary;

    struct {
      struct __NoxAST_NODE* identifier;
      struct __NoxAST_NODE* value;

      __NoxValue_BOOLEAN_TYPE explicit_type;
      __NoxVar_value_type type;
    } assignment;

    struct {
      struct __NoxAST_NODE* condition;
      __NoxDynaArr__* then_branch;
      __NoxDynaArr__* else_branch;
    } if_statement;
  
  } data;
} __NoxAST_NODE;

  typedef
struct {
  __NoxDynaArr__* statements;
} __NoxAST_PROGRAM;

typedef struct {
  size_t count;
  size_t pos;
  __NoxToken__* tokens;
  __NoxError_context* errors;
} parser_info;