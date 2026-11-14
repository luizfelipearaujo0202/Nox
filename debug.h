#pragma once

// debug.h

#include "structures/ast_structures.h"
#include "structures/exec_structures.h"
#include "structures/structures.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static inline const char* 
  __NoxDebug_show_token(const __NoxToken__* t)
  {
  if (t == NULL) return "NULL_TOKEN";
  switch (t->_type) {
    case NEW_LINE: return "NEW_LINE";

    case PLUS: return "PLUS";
    case MINUS: return "MINUS";
    case STAR: return "STAR";
    case SLASH: return "SLASH";
    case PERCENT: return "PERCENT";

    case ASSIGN: return "ASSIGN";
    case PLUS_ASSIGN: return "PLUS_ASSIGN";
    case MINUS_ASSIGN: return "MINUS_ASSIGN";
    case STAR_ASSIGN: return "STAR_ASSIGN";
    case SLASH_ASSIGN: return "SLASH_ASSIGN";
    case PERCENT_ASSIGN: return "PERCENT_ASSIGN";

    case EQUAL: return "EQUAL";
    case NOT_EQUAL: return "NOT_EQUAL";
    case LESS: return "LESS";
    case LESS_EQUAL: return "LESS_EQUAL";
    case GREATER: return "GREATER";
    case GREATER_EQUAL: return "GREATER_EQUAL";

    case SHOW_LINE: return "SHOW_LINE";

    case LPAREN: return "LPAREN";
    case RPAREN: return "RPAREN";
    case LBRACE: return "LBRACE";
    case RBRACE: return "RBRACE";
    case PIPE: return "PIPE";

    case LITERAL: return "LITERAL";
    case IDENTIFIER: return "IDENTIFIER";

    case IF: return "IF";
    case ELSE: return "ELSE";
    case WHILE: return "WHILE";
    case RETURN: return "RETURN";

    case AND: return "AND";
    case OR: return "OR";
    case NOT: return "NOT";

    case EOF_TOKEN: return "EOF_TOKEN";
    case INVALID_TOKEN: return "INVALID";
    default: return "UNKNOWN_TOKEN";
  }
}


static inline const char*
  __NoxDebug_show_token_type(const __NoxToken__* t) 
{
  if (t == NULL) return "NULL_TOKEN";
  switch (t->_literal_type) {
    case VALUE_NONE: return "VALUE_NONE";
    case VALUE_NULL_LITERAL: return "VALUE_NULL_LITERAL";
    case VALUE_INT_LITERAL: return "VALUE_INT_LITERAL";
    case VALUE_FLOAT_LITERAL: return "VALUE_FLOAT_LITERAL";
    case VALUE_STRING_LITERAL: return "VALUE_STRING_LITERAL";
    case VALUE_TRUE: return "VALUE_TRUE";
    case VALUE_FALSE: return "VALUE_FALSE";
    default: return "UNKNOWN_TYPE";
  }
}

static inline const char* 
  __NoxDebug_show_ast_type(const __NoxAST_NODE* n)
  {
  if (n == NULL) return "NULL_NODE";
  switch(n->_type) {
    case AST_LITERAL: return "AST_LITERAL";
    case AST_IDENTIFIER: return "AST_IDENTIFIER";
    case AST_UNARY: return "AST_UNARY";
    case AST_BINARY: return "AST_BINARY";
    case AST_ASSIGNMENT: return "AST_ASSIGNMENT";
    default: return "UNKNOWN_AST_TYPE";
  }
}

static inline const char* 
  __NoxDebug_show_ast_operator(const __NoxAST_NODE* n) {
  if (n == NULL) return "NULL_NODE";
  __NoxAST_OPERATOR op = n->data.binary.op;
  switch (op) {
    case AST_DIVIDE: return "AST_DIVIDE";
    case AST_MINUS: return "AST_MINUS";
    case AST_MULTIPLY: return "AST_MULTIPLY";
    case AST_PLUS: return "AST_PLUS";
    default: return "UNKNOWN_OPERATOR";
  }
}

static inline const char* 
  __NoxDebug_show_calc_status(const __NoxExec_CALC_STATUS s)
  {
  switch (s) {
    case CALC_OK: return "CALC_OK";
    case CALC_INVALID_ACCESS: return "CALC_INVALID_ACCESS";
    case CALC_INVALID_AST_TYPE: return "CALC_INVALID_AST_TYPE";
    case CALC_INVALID_EXPRESSION: return "CALC_INVALID_EXPRESSION";
    case CALC_OUT_OF_MEMORY: return "CALC_OUT_OF_MEMORY";
    case CALC_A_NOT_NUMBER: return "CALC_A_NOT_NUMBER";
    case CALC_B_NOT_NUMBER: return "CALC_B_NOT_NUMBER";
    case CALC_DIVISION_BY_ZERO: return "CALC_DIVISION_BY_ZERO";
    default: return "UNKNOWN_CALC_STATUS";
  }
}


/*
 * DEBUG TREE HELPERS
 */

static inline void
__NoxDebug_tree_line(
  const char* caller,
  const char* prefix,
  const char* branch,
  const char* content
) {
  _NOX_DEBUG_LOG(
    DEBUG_ALL,
    "",
    "",
    "[DEBUG - %s] %s%s%s\n",
    caller,
    prefix,
    branch,
    content
  );
}

static inline const char*
__NoxDebug_value_type_name(__NoxVar_value_type type) {
  switch (type) {
    case NONE_VALUE:   return "NONE";
    case NULL_VALUE:   return "NULL";
    case INT_VALUE:    return "INT";
    case FLOAT_VALUE:  return "FLOAT";
    case STRING_VALUE: return "STRING";
    case BOOL_VALUE:   return "BOOL";
    default:           return "UNKNOWN";
  }
}

static inline const char*
__NoxDebug_ast_type_name(__NoxAST_TYPE type) {
  switch (type) {
    case AST_LITERAL:     return "LITERAL";
    case AST_IDENTIFIER:  return "IDENTIFIER";
    case AST_BINARY:      return "BINARY";
    case AST_UNARY:       return "UNARY";
    case AST_ASSIGNMENT:  return "ASSIGNMENT";
    default:              return "UNKNOWN";
  }
}

static inline const char*
__NoxDebug_operator_name(__NoxAST_OPERATOR op) {
  switch (op) {
    case AST_PLUS:      return "PLUS";
    case AST_MINUS:     return "MINUS";
    case AST_MULTIPLY:  return "MULTIPLY";
    case AST_DIVIDE:    return "DIVIDE";
    default:            return "UNKNOWN";
  }
}

static inline const char*
__NoxDebug_token_type_name(__NoxToken_TYPE type) {
  switch (type) {
    case IDENTIFIER: return "IDENTIFIER";
    case LITERAL:    return "LITERAL";
    case NEW_LINE:   return "NEW_LINE";
    case EOF_TOKEN:  return "EOF_TOKEN";
    case INVALID_TOKEN: return "INVALID_TOKEN";
    default:         return "OTHER";
  }
}

static inline const char*
__NoxDebug_literal_type_name(__NoxToken_LITERAL_TYPE type) {
  switch (type) {
    case VALUE_NONE:          return "NONE";
    case VALUE_INT_LITERAL:   return "INT";
    case VALUE_FLOAT_LITERAL: return "FLOAT";
    case VALUE_STRING_LITERAL:return "STRING";
    case VALUE_TRUE:          return "TRUE";
    case VALUE_FALSE:         return "FALSE";
    case VALUE_NULL_LITERAL:  return "NULL";
    default:                  return "UNKNOWN";
  }
}

/*
 * VARIABLE
 */

static inline void
__NoxDebug_show_var_internal(
  const char* caller,
  const char* prefix,
  const char* branch,
  const __NoxVar__* var
) {
  char buffer[256];

  __NoxDebug_tree_line(
    caller, prefix, branch, "Variável"
  );

  if (var == NULL) {
    __NoxDebug_tree_line(
      caller, prefix, "    ", "NULL"
    );
    return;
  }

  snprintf(
    buffer, sizeof(buffer),
    "Nome: \"%s\"",
    var->name ? var->name : "(null)"
  );

  __NoxDebug_tree_line(
    caller, prefix, "    ├── ", "Identificador"
  );

  __NoxDebug_tree_line(
    caller, prefix, "    │   ├── ", buffer
  );

  snprintf(
    buffer, sizeof(buffer),
    "Endereço: %p",
    (void*)var
  );

  __NoxDebug_tree_line(
    caller, prefix, "    │   ├── ", buffer
  );

  snprintf(
    buffer, sizeof(buffer),
    "Ponteiro do nome: %p",
    (void*)var->name
  );

  __NoxDebug_tree_line(
    caller, prefix, "    │   └── ", buffer
  );

  __NoxDebug_tree_line(
    caller, prefix, "    ├── ", "Configuração"
  );

  snprintf(
    buffer, sizeof(buffer),
    "Tipo explícito: %s",
    var->explicit_type
        ? "TRUE"
        : "FALSE"
  );

  if (!var->explicit_type) {
    __NoxDebug_tree_line(
      caller, prefix, "    │   └── ", buffer
    );
  } else {
    __NoxDebug_tree_line(
      caller, prefix, "    │   ├── ", buffer
    );

    snprintf(
      buffer, sizeof(buffer),
      "Tipo declarado: %s",
      __NoxDebug_value_type_name(var->declared_type)
    );

    __NoxDebug_tree_line(
      caller, prefix, "    │   └── ", buffer
    );
  }

  __NoxDebug_tree_line(
    caller, prefix, "    └── ", "Valor"
  );

  if (var->value == NULL) {
    __NoxDebug_tree_line(
        caller, prefix, "        └── ",
        "Valor: NULL"
    );
    return;
  }

  snprintf(
    buffer, sizeof(buffer),
    "Endereço: %p",
    (void*)var->value
  );

  __NoxDebug_tree_line(
    caller, prefix, "        ├── ", "Estrutura"
  );

  __NoxDebug_tree_line(
    caller, prefix, "        │   └── ",
      buffer
  );

  snprintf(
    buffer, sizeof(buffer),
    "Ref count: %zu",
    var->value->ref_count
  );

  __NoxDebug_tree_line(
    caller, prefix, "        ├── ", buffer
  );

  __NoxDebug_tree_line(
    caller, prefix, "        ├── ", "Tipo"
  );

  snprintf(
    buffer, sizeof(buffer),
    "Tipo geral: %s",
    __NoxDebug_value_type_name(var->value->_t)
  );

  __NoxDebug_tree_line(
    caller, prefix, "        │   └── ", buffer
  );

  __NoxDebug_tree_line(
    caller, prefix, "        └── ", "Conteúdo"
  );

  switch (var->value->_t) {
    case INT_VALUE:
        snprintf(
            buffer, sizeof(buffer),
            "Valor: %ld",
            var->value->_v.v_int
        );
        break;

    case FLOAT_VALUE:
        snprintf(
            buffer, sizeof(buffer),
            "Valor: %.17g",
            var->value->_v.v_float
        );
        break;

    case STRING_VALUE:
        snprintf(
            buffer, sizeof(buffer),
            "Valor: %s",
            var->value->_v.v_string
                ? var->value->_v.v_string
                : "(null)"
        );
        break;

    case BOOL_VALUE:
        snprintf(
            buffer, sizeof(buffer),
            "Valor: %s",
            var->value->_v.v_bool
                ? "TRUE"
                : "FALSE"
        );
        break;

    case NONE_VALUE:
        snprintf(buffer, sizeof(buffer), "Valor: NONE");
        break;

    case NULL_VALUE:
        snprintf(buffer, sizeof(buffer), "Valor: NULL");
        break;

    default:
        snprintf(buffer, sizeof(buffer), "Valor: UNKNOWN");
        break;
  }

  __NoxDebug_tree_line(
      caller, prefix, "            └── ", buffer
  );
}

static inline void
__NoxDebug_show_var(const __NoxVar__* var) {
  __NoxDebug_show_var_internal(
      __func__, "", "", var
  );
}

/*
 * HASH TABLE
 */

static inline void
__NoxDebug_show_table(
    const __NoxVar_HashTable* table
) {
  char buffer[256];

  const char* caller = __func__;

  __NoxDebug_tree_line(
      caller, "", "", "Hash Table"
  );

  if (table == NULL) {
    __NoxDebug_tree_line(
        caller, "", "└── ", "NULL"
    );
    return;
  }

  snprintf(
      buffer, sizeof(buffer),
      "Endereço: %p",
      (void*)table
  );

  __NoxDebug_tree_line(
      caller, "", "├── ", buffer
  );

  snprintf(
      buffer, sizeof(buffer),
      "Capacidade: %zu",
      table->capacity
  );

  __NoxDebug_tree_line(
      caller, "", "├── ", buffer
  );

  snprintf(
      buffer, sizeof(buffer),
      "Elementos: %zu",
      table->count
  );

  __NoxDebug_tree_line(
      caller, "", "├── ", buffer
  );

  snprintf(
      buffer, sizeof(buffer),
      "Tamanho inicial do bucket: %zu",
      table->_bucket_size
  );

  __NoxDebug_tree_line(
      caller, "", "├── ", buffer
  );

  snprintf(
      buffer, sizeof(buffer),
      "Limite por bucket: %zu",
      table->max_bucket_size
  );

  __NoxDebug_tree_line(
      caller, "", "└── ", "Buckets"
  );

  if (table->buckets == NULL) {
    __NoxDebug_tree_line(
        caller, "", "    └── ", "NULL"
    );
    return;
  }

  for (size_t i = 0; i < table->capacity; i++) {
    const __NoxVar__* current = table->buckets[i];

    snprintf(
        buffer, sizeof(buffer),
        "[%zu]",
        i
    );

    __NoxDebug_tree_line(
        caller, "    ",
        i == table->capacity - 1
            ? "└── "
            : "├── ",
        buffer
    );

    if (current == NULL) {
      __NoxDebug_tree_line(
          caller, "    ",
          i == table->capacity - 1
              ? "    "
              : "│   ",
          "EMPTY"
      );
      continue;
    }

    size_t chain_index = 0;

    while (current != NULL) {
      snprintf(
          buffer, sizeof(buffer),
          "Node %zu: \"%s\" | %p",
          chain_index,
          current->name ? current->name : "(null)",
          (void*)current
      );

      __NoxDebug_tree_line(
          caller, "    ",
          current->next ? "│   ├── " : "│   └── ",
          buffer
      );

      current = current->next;
      chain_index++;
    }
  }
}




/*
 * AST
 */

static inline void
__NoxDebug_show_ast_node(
    const char* caller,
    const char* prefix,
    const char* branch,
    int is_last,
    const char* label,
    const __NoxAST_NODE* node,
    size_t depth
) {
  char buffer[256];
  char child_prefix[512];

  __NoxDebug_tree_line(
      caller, prefix, branch, label
  );

  if (node == NULL) {
    __NoxDebug_tree_line(
        caller, prefix, "    └── ", "NULL"
    );
    return;
  }

  if (depth >= 128) {
    __NoxDebug_tree_line(
        caller, prefix, "    └── ",
        "Maximum recursion depth reached"
    );
    return;
  }

  snprintf(
      buffer, sizeof(buffer),
      "Tipo: %s (%d)",
      __NoxDebug_ast_type_name(node->_type),
      (int)node->_type
  );

  __NoxDebug_tree_line(
      caller, prefix, "    ├── ", buffer
  );

  switch (node->_type) {
    case AST_LITERAL:
    case AST_IDENTIFIER: {
      const __NoxToken__* token = node->data.token;

      if (token == NULL) {
        __NoxDebug_tree_line(
            caller, prefix, "    └── ",
            "Token: NULL"
        );
        break;
      }

      snprintf(
          buffer, sizeof(buffer),
          "Endereço: %p",
          (void*)token
      );

      __NoxDebug_tree_line(
          caller, prefix, "    ├── ", "Token"
      );

      __NoxDebug_tree_line(
          caller, prefix, "    │   ├── ", buffer
      );

      snprintf(
          buffer, sizeof(buffer),
          "Tipo: %s",
          __NoxDebug_token_type_name(token->_type)
      );

      __NoxDebug_tree_line(
          caller, prefix, "    │   ├── ", buffer
      );

      snprintf(
          buffer, sizeof(buffer),
          "Tipo literal: %s",
          __NoxDebug_literal_type_name(
              token->_literal_type
          )
      );

      __NoxDebug_tree_line(
          caller, prefix, "    │   ├── ", buffer
      );

      snprintf(
          buffer, sizeof(buffer),
          "Lexema: \"%.*s\"",
          (int)token->length,
          token->start ? token->start : ""
      );

      __NoxDebug_tree_line(
          caller, prefix, "    │   ├── ", buffer
      );

      snprintf(
          buffer, sizeof(buffer),
          "Linha: %zu | Coluna: %zu",
          token->line,
          token->column
      );

      __NoxDebug_tree_line(
          caller, prefix, "    │   └── ", buffer
      );

      break;
    }

    case AST_BINARY: {
      snprintf(
          buffer, sizeof(buffer),
          "Operador: %s",
          __NoxDebug_operator_name(
              node->data.binary.op
          )
      );

      __NoxDebug_tree_line(
          caller, prefix, "    ├── ", buffer
      );

      snprintf(
          child_prefix, sizeof(child_prefix),
          "%s%s",
          prefix,
          is_last ? "    " : "│   "
      );

      __NoxDebug_show_ast_node(
          caller,
          child_prefix,
          "├── ",
          0,
          "Left",
          node->data.binary.left,
          depth + 1
      );

      __NoxDebug_show_ast_node(
          caller,
          child_prefix,
          "└── ",
          1,
          "Right",
          node->data.binary.right,
          depth + 1
      );

      break;
    }

    case AST_ASSIGNMENT: {
      snprintf(
          child_prefix, sizeof(child_prefix),
          "%s%s",
          prefix,
          is_last ? "    " : "│   "
      );

      __NoxDebug_show_ast_node(
          caller,
          child_prefix,
          "├── ",
          0,
          "Identifier",
          node->data.assignment.identifier,
          depth + 1
      );

      __NoxDebug_show_ast_node(
          caller,
          child_prefix,
          "└── ",
          1,
          "Value",
          node->data.assignment.value,
          depth + 1
      );

      break;
    }

    case AST_UNARY:
      __NoxDebug_tree_line(
          caller, prefix, "    └── ",
          "Payload not implemented in AST structure"
      );
      break;

    default:
      __NoxDebug_tree_line(
          caller, prefix, "    └── ",
          "Unknown AST node"
      );
      break;
  }
}

static inline void
__NoxDebug_show_ast(const __NoxAST_NODE* node) {
  __NoxDebug_show_ast_node(
      __func__,
      "",
      "",
      1,
      "AST_NODE",
      node,
      0
  );
}

/*
 * AST PROGRAM
 */

static inline void
__NoxDebug_show_program(
    const __NoxAST_PROGRAM* program
) {
  char buffer[256];

  const char* caller = __func__;

  __NoxDebug_tree_line(
      caller, "", "", "AST_PROGRAM"
  );

  if (program == NULL) {
    __NoxDebug_tree_line(
        caller, "", "└── ", "NULL"
    );
    return;
  }

  snprintf(
      buffer, sizeof(buffer),
      "Endereço: %p",
      (void*)program
  );

  __NoxDebug_tree_line(
      caller, "", "├── ", buffer
  );

  if (program->statements == NULL) {
    __NoxDebug_tree_line(
        caller, "", "└── ",
        "Statements: NULL"
    );
    return;
  }

  const __NoxDynaArr__* arr = program->statements;

  snprintf(
      buffer, sizeof(buffer),
      "Statements: %zu",
      arr->count
  );

  __NoxDebug_tree_line(
      caller, "", "├── ", buffer
  );

  if (arr->data == NULL ||
      arr->element_size < sizeof(__NoxAST_NODE*)) {
    __NoxDebug_tree_line(
        caller, "", "└── ",
        "Invalid statements array"
    );
    return;
  }

  __NoxDebug_tree_line(
      caller, "", "└── ", "Nodes"
  );

  __NoxAST_NODE** nodes =
      (__NoxAST_NODE**)arr->data;

  for (size_t i = 0; i < arr->count; i++) {
    snprintf(
        buffer, sizeof(buffer),
        "Statement [%zu]",
        i
    );

    __NoxDebug_show_ast_node(
        caller,
        "    ",
        i == arr->count - 1 ? "└── " : "├── ",
        i == arr->count - 1,
        buffer,
        nodes[i],
        0
    );
  }
}