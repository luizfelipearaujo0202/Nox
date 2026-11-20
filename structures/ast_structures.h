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
AST_GREATER_EQUAL,// >=

AST_AND,
AST_OR
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
      __NoxDynaArr__* then_branch; // Array of NODE*
      __NoxDynaArr__* else_branch; // Array of NODE*
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


/*
Função auxiliar para coleção do
primeiro token de um node
*/
static inline const __NoxToken__*
__NoxAST_get_first_token(const __NoxAST_NODE* node) {
  if (node == NULL)
    { return NULL; }

  switch(node->_type) {
    case AST_LITERAL:

    case AST_IDENTIFIER:
      return node->data.token;

    case AST_BINARY:
      return __NoxAST_get_first_token(
        node->data.binary.left
      );

    default: return NULL;
  }
}