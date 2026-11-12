#pragma once

#include "../parser/var/var_table.h"
#include "structures.h"
#include "ast_structures.h"
#include "../dynamic_array.h"

typedef enum {
CALC_OK,
CALC_A_NOT_NUMBER,
CALC_B_NOT_NUMBER,
CALC_DIVISION_BY_ZERO,
CALC_INVALID_EXPRESSION,
CALC_INVALID_AST_TYPE,
CALC_INVALID_ACCESS,
CALC_OUT_OF_MEMORY
} __NoxExec_CALC_STATUS;

typedef struct {
  __NoxExec_CALC_STATUS status;
  long result;
} __NoxExec_CALC_RESULT;

// Context
typedef struct {
  __NoxAST_PROGRAM* program;
  size_t index;
  __NoxAST_NODE* node;
  __NoxVar_HashTable* table;
  __NoxError_context* errors;
} __NoxExec_context;

static inline __NoxValue_BOOLEAN_TYPE
  __NoxExec_context_is_valid(
    __NoxExec_context* c
  ) {
    if (c != NULL &&
        c->program != NULL &&
        __NoxDynaArr_is_valid(c->program->statements) &&
        c->table != NULL)
      {return __NoxBool_TRUE_VALUE;}
    else return __NoxBool_FALSE_VALUE;
  }