#pragma once

#include "debug.h"

#include "error.h"
#include "error/error_registry.h"
#include "error/error_structures.h"

#include "parser/var/var.h"
#include "parser/var/var_table.h"

#include "structures/ast_structures.h"
#include "structures/exec_structures.h"
#include "structures/structures.h"

#include "dynamic_array.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static inline __NoxExec_CALC_RESULT
__NoxExec_calculate(__NoxExec_context* c)
{
  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "EXECUTOR",
    "calculate",
    "Inicializado\n"
  );

  __NoxExec_CALC_RESULT r;
  r.status = CALC_OK;
  r.result = 0;
  _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "calculate", "__NoxExec_CALC_RESULT 'r' criado\n");
  if (!__NoxExec_context_is_valid(c)) {
    _NOX_DEBUG_LOG(DEBUG_ERROR, "EXECUTOR", "calculate", "context invalid\n");
    r.status = CALC_INVALID_ACCESS;
    return r;
  }

  __NoxAST_NODE* node = c->node;
  if (node == NULL) {
       node = *(__NoxAST_NODE**)\
      __NoxDynaArr_get(c->program->statements, c->index);

    if (node == NULL) {
      _NOX_DEBUG_LOG(DEBUG_VERIFY, "EXECUTOR", "calculate", "argumentos [NULL]\n");
      r.status = CALC_INVALID_ACCESS;
      return r;
    }
    c->node = node;
  }

  if (node == NULL) {
    _NOX_DEBUG_LOG(DEBUG_VERIFY, "EXECUTOR", "calculate", "argumentos [NULL]\n");
    r.status = CALC_INVALID_ACCESS;
    return r;
  }

  __NoxExec_context left_side = {
    .program = c->program,
    .index = c->index,
    .node = node->data.binary.left,
    .table = c->table,
    .errors = c->errors
  };
  
  __NoxExec_context right_side = {
    .program = c->program,
    .index = c->index,
    .node = node->data.binary.right,
    .table = c->table,
    .errors = c->errors
  };

  if (node->_type != AST_BINARY) {
    _NOX_DEBUG_LOG(DEBUG_VERIFY, "EXECUTOR", "calculate", "argumentos [NOT BINARY]\n");
    r.status = CALC_INVALID_AST_TYPE;
    return r;
  }
  _NOX_DEBUG_LOG(DEBUG_VERIFY, "EXECUTOR", "calculate", "argumentos [OK]\n");

  _NOX_DEBUG_LOG(DEBUG_VERIFY, "EXECUTOR", "calculate", "sides:\n  Left: %s  Right: %s",
    (node->data.binary.left == NULL ? "[NULL]\n" : "[OK]\n"),
    (node->data.binary.right == NULL ? "[NULL]\n" : "[OK]\n")
  );

  if (
    (
      node->data.binary.left == NULL ||
      node->data.binary.left->data.token == NULL
    ) || (
      node->data.binary.right == NULL ||
      node->data.binary.right->data.token == NULL
    )
  ) { r.status = CALC_INVALID_ACCESS; return r; }

  __NoxExec_CALC_RESULT left_eval;
  __NoxExec_CALC_RESULT right_eval;

  _NOX_DEBUG_LOG(DEBUG_VERIFY, "EXECUTOR", "calculate", "side's type:\n  Left: %s\n  Right: %s\n",
    show_ast_type(node->data.binary.left),
    show_ast_type(node->data.binary.right)
  );

  if (node->data.binary.left->_type == AST_BINARY)
  {
    left_eval = __NoxExec_calculate(&left_side);
    if (left_eval.status != CALC_OK)
      { r.status = left_eval.status; return r; }
  }
  else if ( node->data.binary.left->_type == AST_LITERAL &&
            node->data.binary.left->data.token->_literal_type != VALUE_INT_LITERAL)
  { }
  else
  { r.status = CALC_A_NOT_NUMBER; return r; }

  if (node->data.binary.right->_type == AST_BINARY)
  {
    right_eval = __NoxExec_calculate(&right_side);
    if (right_eval.status != CALC_OK)
      { r.status = right_eval.status; return r; }
  }
  else if ( node->data.binary.right->_type == AST_LITERAL &&
            node->data.binary.right->data.token->_literal_type != VALUE_INT_LITERAL)
  { r.status = CALC_B_NOT_NUMBER; return r; }

  long left = 0;
  long right = 0;

  if (node->data.binary.left->_type == AST_LITERAL &&
      node->data.binary.left->data.token->_literal_type != VALUE_INT_LITERAL)
  {
    if (node->data.binary.left->data.token == NULL ||
        node->data.binary.left->data.token->start == NULL)
      { r.status = CALC_INVALID_ACCESS; return r;}

    size_t left_length = node->data.binary.left->data.token->length;  
    char* left_str = (char*)malloc(left_length + 1);  
    if (left_str == NULL)   
      { r.status = CALC_OUT_OF_MEMORY;   
        return r; }  

    memcpy(left_str, node->data.binary.left->data.token->start, left_length);  
    left_str[left_length] = '\0';  
    left = strtol(left_str, NULL, 10);  
    free(left_str);  

    if (errno == ERANGE) {  
      r.status = CALC_INVALID_ACCESS;  
      return r;  
    }
  }
  else if (node->data.binary.left->_type == AST_BINARY)
  { left = left_eval.result; }

  if (node->data.binary.right->_type == AST_LITERAL &&
      node->data.binary.right->data.token->_literal_type != VALUE_INT_LITERAL)
  {
    if (node->data.binary.right->data.token == NULL ||
        node->data.binary.right->data.token->start == NULL)
      { r.status = CALC_INVALID_ACCESS; return r;}

    size_t right_length = node->data.binary.right->data.token->length;  
    char* right_str = (char*)malloc(right_length + 1);  
    if (right_str == NULL)  
      { r.status = CALC_OUT_OF_MEMORY;   
        return r; }  

    memcpy(right_str, node->data.binary.right->data.token->start, right_length);  
    right_str[right_length] = '\0';  
    right = strtol(right_str, NULL, 10);  
    free(right_str);  

    if (errno == ERANGE) {  
      r.status = CALC_INVALID_ACCESS;  
      return r;  
    }
  }
  else if (node->data.binary.right->_type == AST_BINARY)
  { right = right_eval.result; }

  long res = 0;

  _NOX_DEBUG_LOG(DEBUG_VERIFY, "EXECUTOR", "calculate", "\noperator: %s", show_ast_operator(node));
  
  if (node->data.binary.op == AST_PLUS) 
    { res = left + right; } 
  else if (node->data.binary.op == AST_MINUS)
    { res = left - right; }
  else if (node->data.binary.op == AST_MULTIPLY)
    { res = left * right; }
  else if (node->data.binary.op == AST_DIVIDE)
  { 
    if (right == 0) 
      { r.status = CALC_DIVISION_BY_ZERO; return r; }
    else 
      { res = left / right; } 
  }
  else 
    { r.status = CALC_INVALID_EXPRESSION; return r; }

  r.result = res;
  _NOX_DEBUG_LOG(DEBUG_VERIFY, "EXECUTOR", "calculate", "\n");
  return r;
}

static inline __NoxValue_BOOLEAN_TYPE
__NoxExec_read_identifier(__NoxExec_context* c) 
{
  if (!__NoxExec_context_is_valid(c))
    { _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "EXECUTOR",
        "read_identifier",
        "contexto inválido\n"
      );
      return __NoxBool_FALSE_VALUE; }

  __NoxAST_NODE* node = c->node;
  if (node == NULL) {
       node = *(__NoxAST_NODE**)\
      __NoxDynaArr_get(c->program->statements, c->index);
    if (node == NULL) {
      _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "EXECUTOR",
        "read_identifier",
        "node = null"
      );
    }
    c->node = node;
  }
  if (node->_type != AST_IDENTIFIER)
    { _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "EXECUTOR",
        "read_identifier",
        "Tipo da AST diferente de IDENTIFIER\n"
      );
      return __NoxBool_FALSE_VALUE; }

  size_t key_len = node->data.token->length;
  char* key = (char*)malloc(key_len + 1);
  _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "read_identifier", "alocando 'key': ");
  if (key == NULL)
    { _NOX_DEBUG_LOG(DEBUG_ERROR, "EXECUTOR", "read_identifier", "[ERRO]\n");
      return __NoxBool_FALSE_VALUE; }
  _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "read_identifier", "[DONE]\n");

  memcpy(key, node->data.token->start, key_len);
  key[key_len] = '\0';

  __NoxVar__* var = __NoxVar_HashTable_get(c->table, key);
  if (var == NULL) {
    _NOX_DEBUG_LOG(DEBUG_ERROR, "EXECUTOR", "read_identifier", "[ERRO]\n");
    if (c->errors != NULL && c->errors->registry != NULL) {
      char msg[128];
      snprintf(
        msg, 
        sizeof(msg), 
        "variable '%.*s' not declared", 
        (int)key_len, key
      );

      __NoxError__* e = __NoxError_create(
        ERROR_RUNTIME, msg,
        (int)node->data.token->line,
        (int)node->data.token->column,
        (int)node->data.token->column + node->data.token->length
      );

      __NoxError_register(c->errors->registry, e, __NoxBool_TRUE_VALUE);
    }
    free(key);
    return __NoxBool_FALSE_VALUE;
  }

  free(key);
  return __NoxBool_TRUE_VALUE;
}

static inline __NoxValue_BOOLEAN_TYPE
__NoxExec_write_identifier(__NoxExec_context* c)
{
  if (!__NoxExec_context_is_valid(c))
    { return __NoxBool_FALSE_VALUE; }

  __NoxAST_NODE* node = c->node;
  if (node == NULL) {
     node = *(__NoxAST_NODE**)\
      __NoxDynaArr_get(c->program->statements, c->index);
    if (node == NULL) {
      _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "EXECUTOR",
        "write_identifier",
        "node = null"
      );
      return __NoxBool_FALSE_VALUE;
    }
    c->node = node;
  } 

  _NOX_DEBUG_LOG(
    DEBUG_VERIFY,
    "EXECUTOR",
    "write_identifier",
    "node: %p | type: %s\n",
    (void*)node,
    show_ast_type(node)
  );
  _NOX_DEBUG_LOG(
    DEBUG_VERIFY,
    "EXECUTOR",
    "write_identifier",
    "assignment.identifier: %p | assignment.value: %p\n",
    (void*)node->data.assignment.identifier,
    (void*)node->data.assignment.value
  );

  if (node->_type != AST_ASSIGNMENT)
    { return __NoxBool_FALSE_VALUE; }

  // 327
  if (
    node->data.assignment.identifier == NULL ||
    node->data.assignment.value == NULL
  ) { return __NoxBool_FALSE_VALUE; }

  if (
    node->data.assignment.identifier->data.token == NULL ||
    node->data.assignment.value->data.token == NULL
  ) { return __NoxBool_FALSE_VALUE; }

  size_t key_len = node->data.assignment.identifier->data.token->length;
  char* key = (char*)malloc(key_len + 1);
  if (key == NULL)
    { return __NoxBool_FALSE_VALUE; }
  memcpy(key, node->data.assignment.identifier->data.token->start, key_len);
  key[key_len] = '\0';

  size_t value_len = node->data.assignment.value->data.token->length;
  char* value_str = (char*)malloc(value_len + 1);
  if (value_str == NULL)
    { free(key);
      return __NoxBool_FALSE_VALUE; }
  memcpy(value_str, node->data.assignment.value->data.token->start, value_len);
  value_str[value_len] = '\0';

  __NoxVar_value* value = NULL;
  switch (node->data.assignment.value->data.token->_literal_type)
  {
    case VALUE_NONE: {
      value = __NoxVar_value_create(NONE_VALUE, (__NoxVar_v){.v_none = __NoxBool_TRUE_VALUE});
    } break;

    case VALUE_NULL_LITERAL: {  
      value = __NoxVar_value_create(NULL_VALUE, (__NoxVar_v){.v_null = __NoxBool_TRUE_VALUE});  
    } break;  

    case VALUE_STRING_LITERAL: {  
      value = __NoxVar_value_create(STRING_VALUE, (__NoxVar_v){.v_string = value_str});  
    } break;  

    case VALUE_INT_LITERAL: {  
      value = __NoxVar_value_create(INT_VALUE, (__NoxVar_v){.v_int = strtol(value_str, NULL, 10)});  
    } break;  

    case VALUE_FLOAT_LITERAL: {  
      value = __NoxVar_value_create(FLOAT_VALUE, (__NoxVar_v){.v_float = (double)strtof(value_str, NULL)});  
    } break;  

    case VALUE_TRUE: {  
      value = __NoxVar_value_create(BOOL_VALUE, (__NoxVar_v){.v_bool = __NoxBool_TRUE_VALUE});  
    } break;  

    case VALUE_FALSE: {  
      value = __NoxVar_value_create(BOOL_VALUE, (__NoxVar_v){.v_bool = __NoxBool_FALSE_VALUE});  
    } break;
  }

  if (value != NULL) {
    value->ref_count = 0;
  }

  __NoxVar__* var = __NoxVar_create(key, value, NULL);
  __NoxVar_HashTable_insert(c->table, var);
  __NoxVar_free(var);

  free(key);
  free(value_str);

  return __NoxBool_TRUE_VALUE;
}

static inline __NoxValue_BOOLEAN_TYPE
__NoxExec_process_single(__NoxExec_context* c, size_t index) 
{
  _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "process_single", "Iniciado\n");
  if (!__NoxExec_context_is_valid(c))
    { _NOX_DEBUG_LOG(DEBUG_ERROR, "EXECUTOR", "process_single", "Contexto inválido\n");
      return __NoxBool_FALSE_VALUE; }

  __NoxAST_NODE* node = c->node;
  if (node == NULL) {
      node = *(__NoxAST_NODE**)\
      __NoxDynaArr_get(c->program->statements, index);
    if (node == NULL) {
      _NOX_DEBUG_LOG(DEBUG_ERROR, "EXECUTOR", "process_single", "Nó da AST é NULL\n");
      return __NoxBool_FALSE_VALUE;
    }
    c->index = index;
    c->node = node;
  }

  _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "process_single", "Escolhendo workers\n");
  switch(node->_type) {
    case AST_ASSIGNMENT: {
      _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "process_single", "Executando 'write_identifier'\n");
      if (!__NoxExec_write_identifier(c))
        { _NOX_DEBUG_LOG(DEBUG_ERROR, "", "", "[ERRO]\n");
          return __NoxBool_FALSE_VALUE; }
    } break;

    case AST_IDENTIFIER: {  
      _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "process_single", "Executando 'read_identifier'\n");  
      if (!__NoxExec_read_identifier(c))  
        { _NOX_DEBUG_LOG(DEBUG_ERROR, "", "", "[ERRO]\n");  
          return __NoxBool_FALSE_VALUE; }  
    } break;  

    case AST_BINARY: {  
      _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "process_single", "Executando 'calculate'\n");  
      if (__NoxExec_calculate(c).status != CALC_OK)  
        { _NOX_DEBUG_LOG(DEBUG_ERROR, "", "", "[ERRO]\n");  
          return __NoxBool_FALSE_VALUE; }  
    } break;  

    default: {  
      _NOX_DEBUG_LOG(DEBUG_ERROR, "EXECUTOR", "process_single", "Tipo da AST desconhecido\n");  
      return __NoxBool_FALSE_VALUE;   
    }
  }

  _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "process_single", "Execução concluída\n");

  return __NoxBool_TRUE_VALUE;
}
