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
#include <stdarg.h>

/*
=======================
====     UTILS     ====
=======================
*/

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

static inline __NoxError__*
__NoxExec_create_error(
    __NoxExec_context* c,
    __NoxError_TYPE type,
    int line,
    int c_start,
    int c_end,
    const char* format,
    ...
) {
  if (!__NoxExec_context_is_valid(c))
    { return NULL; }
  if (
    c->errors == NULL ||
    c->errors->registry == NULL ||
    format == NULL
  )
    { return NULL; }

  char msg[512];
  va_list args;
  va_start(args, format);
  vsnprintf(msg, sizeof(msg), format, args);
  va_end(args);

  __NoxError__* e = __NoxError_create(
    type,
    msg,
    line,
    c_start,
    c_end
  );
  if (e == NULL)
    { return NULL; }

  if (!__NoxError_register(
    c->errors->registry,
    e,
    __NoxBool_TRUE_VALUE
  )) {
    __NoxError_free(e);
    return NULL;
  }
  return e;
}

// =======================

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
    __NoxDebug_show_ast_type(node->data.binary.left),
    __NoxDebug_show_ast_type(node->data.binary.right)
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

  _NOX_DEBUG_LOG(DEBUG_VERIFY, "EXECUTOR", "calculate", "\noperator: %s", 
    __NoxDebug_show_ast_operator(node));
  
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


static inline __NoxVar__*
__NoxExec_read_identifier(__NoxExec_context* c)
{
  if (!__NoxExec_context_is_valid(c))
    { _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "EXECUTOR",
        "read_identifier",
        "contexto inválido\n"
      );
      return NULL; }

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
  if (node->_type != AST_ASSIGNMENT)
    { _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "EXECUTOR",
        "read_identifier",
        "Tipo da AST diferente de ASSIGNMENT\n"
      );
      return NULL; }

  const __NoxToken__* t =
    node->data.assignment.identifier->data.token;

  if (t == NULL) {
    _NOX_DEBUG_LOG(
      DEBUG_ERROR,
      "EXECUTOR",
      "read_identifier",
      "Token do identifier é NULL\n"
    );
    return NULL;
  }

  size_t key_len = t->length;
  char* key = (char*)malloc(t->length + 1);
  _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "read_identifier", "alocando 'key'");
  if (key == NULL)
    { _NOX_DEBUG_LOG(DEBUG_ERROR, "", "", " [ERRO]\n");
      return NULL; }
  _NOX_DEBUG_LOG(DEBUG_VERIFY, "", "", " [DONE]\n");

  memcpy(key, t->start, t->length);
  key[key_len] = '\0';

  _NOX_DEBUG_LOG(
    DEBUG_VERIFY,
    "EXECUTOR",
    "read_identifier",
    "coletando variável: "
  );
  __NoxVar__* var = __NoxVar_HashTable_get(c->table, key);
  if (var == NULL) {
    _NOX_DEBUG_LOG(DEBUG_ERROR, "", "", "[NULL]\n");

    _NOX_DEBUG_LOG(
      DEBUG_VERIFY,
      "EXECUTOR",
      "read_identifier",
      "c->errors = %p | c->errors->registry = %p\n",
      (void*)c->errors,
      (void*)c->errors->registry
    );

    if (c->errors != NULL && c->errors->registry != NULL) {
      _NOX_DEBUG_LOG(
        DEBUG_INFO,
        "EXECUTOR",
        "read_identifier",
        "criando erro de runtime"
      );
      __NoxError__* e =
        __NoxExec_create_error(
          c,
          ERROR_RUNTIME,
          t->line,
          t->column,
          t->column +
            t->length,
          "variable '%.*s' not declared",
          (int)t->length, key
        );

      _NOX_DEBUG_LOG(
        DEBUG_VERIFY,
        "",
        "",
        ": %p\n",
        (void*)e
      );

      __NoxError_free(e);
    }
    free(key);
    return NULL;
  }
  _NOX_DEBUG_LOG(DEBUG_VERIFY, "", "", "[DONE]\n");

  free(key);
  return var;
}

static inline __NoxVar__*
__NoxExec_read_value(__NoxExec_context* c) 
{
  if (!__NoxExec_context_is_valid(c))
    { _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "EXECUTOR",
        "read_value",
        "contexto inválido\n"
      );
      return NULL; }

  __NoxAST_NODE* node = c->node;
  if (node == NULL) {
       node = *(__NoxAST_NODE**)\
      __NoxDynaArr_get(c->program->statements, c->index);
    if (node == NULL) {
      _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "EXECUTOR",
        "read_value",
        "node = null"
      );
    }
    c->node = node;
  }
  if (node->_type != AST_ASSIGNMENT)
    { _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "EXECUTOR",
        "read_value",
        "Tipo da AST diferente de ASSIGNMENT\n"
      );
      return NULL; }

  const __NoxToken__* t =
    node->data.assignment.value->data.token;

  if (t == NULL) {
    _NOX_DEBUG_LOG(
      DEBUG_ERROR,
      "EXECUTOR",
      "read_value",
      "Token do value é NULL\n"
    );
    return NULL;
  }

  size_t key_len = t->length;
  char* key = (char*)malloc(t->length + 1);
  _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "read_value", "alocando 'key'");
  if (key == NULL)
    { _NOX_DEBUG_LOG(DEBUG_ERROR, "", "", " [ERRO]\n");
      return NULL; }
  _NOX_DEBUG_LOG(DEBUG_VERIFY, "", "", " [DONE]\n");

  memcpy(key, t->start, t->length);
  key[key_len] = '\0';

  _NOX_DEBUG_LOG(
    DEBUG_VERIFY,
    "EXECUTOR",
    "read_value",
    "coletando variável: "
  );
  __NoxVar__* var = __NoxVar_HashTable_get(c->table, key);
  if (var == NULL) {
    _NOX_DEBUG_LOG(DEBUG_ERROR, "", "", "[NULL]\n");

    _NOX_DEBUG_LOG(
      DEBUG_VERIFY,
      "EXECUTOR",
      "read_value",
      "c->errors = %p | c->errors->registry = %p\n",
      (void*)c->errors,
      (void*)c->errors->registry
    );

    if (c->errors != NULL && c->errors->registry != NULL) {
      _NOX_DEBUG_LOG(
        DEBUG_INFO,
        "EXECUTOR",
        "read_value",
        "criando erro de runtime"
      );
      __NoxError__* e =
        __NoxExec_create_error(
          c,
          ERROR_RUNTIME,
          t->line,
          t->column,
          t->column +
            t->length,
          "variable '%.*s' not declared",
          (int)t->length, key
        );

      _NOX_DEBUG_LOG(
        DEBUG_VERIFY,
        "",
        "",
        ": %p\n",
        (void*)e
      );

      __NoxError_free(e);
    }
    free(key);
    return NULL;
  }
  _NOX_DEBUG_LOG(DEBUG_VERIFY, "", "", "[DONE]\n");

  free(key);
  return var;
}


/*
!
! WRITE_IDENTIFIER COM ERRO         !
! Precisa conhecer tipagem fixa para!
! registrar erros sobre.            !
!
*/
static inline __NoxValue_BOOLEAN_TYPE
__NoxExec_write_identifier(__NoxExec_context* c)
{
  if (!__NoxExec_context_is_valid(c))
    { return __NoxBool_FALSE_VALUE; }

  __NoxAST_NODE* node = *(__NoxAST_NODE**)\
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

  _NOX_DEBUG_LOG(
    DEBUG_VERIFY,
    "EXECUTOR",
    "write_identifier",
    "node: %p | type: %s\n",
    (void*)node,
    __NoxDebug_show_ast_type(node)
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

  /*
  Verifica se uma variável utilizada no valor existe
  */
  __NoxValue_BOOLEAN_TYPE is_shared =
    __NoxBool_FALSE_VALUE; // Verifica se valor é de var existente
  __NoxVar__* existing_var = NULL;
  if (
    node->data.assignment.value->data.token->_type == IDENTIFIER
  ) { 
    existing_var = __NoxExec_read_value(c);
    if (existing_var != NULL) {
      value = existing_var->value;
      is_shared = __NoxBool_TRUE_VALUE; // veio de outra variavel
    }
    else {
      if (c->errors != NULL && c->errors->registry != NULL) {
        __NoxToken__* t =
          node->data.assignment.value->data.token;
        __NoxError__* e =
          __NoxExec_create_error(
            c,
            ERROR_NAME,
            t->line,
            t->column,
            t->column + t->length,
            "use of undefined identifier '%.*s'",
            (int)t->length, t->start
          );

        __NoxError_free(e);
      }

      free(key);
      free(value_str);
      return __NoxBool_FALSE_VALUE;
    }
  }
  else {
    switch (node->data.assignment.value->data.token->_literal_type)
    {
      case VALUE_NONE: {
        value = __NoxVar_value_create(
          NONE_VALUE, 
          (__NoxVar_v){.v_none = __NoxBool_TRUE_VALUE}
        );
      } break;

      case VALUE_NULL_LITERAL: {  
        value = __NoxVar_value_create(
          NULL_VALUE, 
          (__NoxVar_v){.v_null = __NoxBool_TRUE_VALUE}
        );  
      } break;  

      case VALUE_STRING_LITERAL: {  
        value = __NoxVar_value_create(
          STRING_VALUE, 
          (__NoxVar_v){.v_string = value_str}
        );  
      } break;  

      case VALUE_INT_LITERAL: {  
        value = __NoxVar_value_create(
          INT_VALUE, 
          (__NoxVar_v){.v_int = strtol(value_str, NULL, 10)}
        );  
      } break;  

      case VALUE_FLOAT_LITERAL: {  
        value = __NoxVar_value_create(
          FLOAT_VALUE, 
          (__NoxVar_v){.v_float = (double)strtof(value_str, NULL)}
        );  
      } break;  

      case VALUE_TRUE: {  
        value = __NoxVar_value_create(
          BOOL_VALUE, 
          (__NoxVar_v){.v_bool = __NoxBool_TRUE_VALUE}
        );  
      } break;  

      case VALUE_FALSE: {  
        value = __NoxVar_value_create(
          BOOL_VALUE, 
          (__NoxVar_v){.v_bool = __NoxBool_FALSE_VALUE}
        );  
      } break;
    }
  }

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "EXECUTOR",
    "write_identifier",
    "=== DEBUG DO NODE ATUAL ===\n"
  );
  __NoxDebug_show_ast(c->node);
  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "EXECUTOR",
    "write_identifier",
    "===========================\n"
  );

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "EXECUTOR",
    "write_identifier",
    "coletando variável existente:\n"
  );
  __NoxVar__* existing = __NoxVar_HashTable_get(c->table, key);

  __NoxDebug_show_var(existing);

  if (existing != NULL && existing->explicit_type) {
    if (existing->declared_type != value->_t) {
      // MISMATCH!
      /*
      Variável com tipo declarado explicitamente recebendo
      atribuição de tipo diferente
      */
      _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "EXECUTOR",
        "write_identifier",
        "Mismatch de tipo: '%s' em variável de tipo '%s'\n",
        __NoxDebug_value_type_name(value->_t),
          __NoxDebug_value_type_name(existing->declared_type)
      );
      const __NoxToken__* t =
        node->data.assignment.value->data.token;
      __NoxError__* e = __NoxExec_create_error(
          c, ERROR_RUNTIME, 
          t->line,
          t->column,
          t->column + t->length,
          "cannot assign type '%s' to fixed variable '%s' of type '%s'",
          __NoxDebug_value_type_name(value->_t),
          key,
          __NoxDebug_value_type_name(existing->declared_type)
      );

      _NOX_DEBUG_LOG(
        DEBUG_VERIFY,
        "EXECUTOR",
        "write_identifier",
        "erro criado: %p\n",
        (void*)e
      );

      if (e) __NoxError_free(e);
      
      // Limpa o value novo que não vai ser usado
      if (!is_shared) {
          __NoxVar_value_clear(value);
          free(value);
      }
      free(key);
      free(value_str);
      return __NoxBool_FALSE_VALUE;
    }
  }
  __NoxVar__* new_var = NULL;
  if (existing != NULL) {
    new_var = __NoxVar_create(
      key,
      value,
      existing->explicit_type,
      existing->declared_type,
      NULL
    );
  }
  else {
    new_var = __NoxVar_create(
      key,
      value,
      node->data.assignment.explicit_type,
      node->data.assignment.type,
      NULL
    );
  }
  if (new_var == NULL) {
      if (!is_shared) { __NoxVar_value_clear(value); free(value); }
      free(key); free(value_str);
      return __NoxBool_FALSE_VALUE;
  }

  __NoxVar_HashTable_insert(c->table, new_var);
  
  __NoxDebug_show_var(new_var);

  __NoxVar_free(new_var);
  
  if (!is_shared) {
      __NoxVar_value_clear(value);
      free(value);
  }

  free(value_str);
  free(key);

  return __NoxBool_TRUE_VALUE;
}

static inline __NoxValue_BOOLEAN_TYPE
__NoxExec_process_single(__NoxExec_context* c, size_t index) 
{
  _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "process_single", "Iniciado\n");
  if (!__NoxExec_context_is_valid(c))
    { _NOX_DEBUG_LOG(DEBUG_ERROR, "EXECUTOR", "process_single", "Contexto inválido\n");
      return __NoxBool_FALSE_VALUE; }

  __NoxAST_NODE* node = *(__NoxAST_NODE**)\
    __NoxDynaArr_get(c->program->statements, index);
  if (node == NULL) {
    _NOX_DEBUG_LOG(DEBUG_ERROR, "EXECUTOR", "process_single", "Nó da AST é NULL\n");
    return __NoxBool_FALSE_VALUE;
  }

  _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "process_single", "Escolhendo workers\n");
  switch(node->_type) {
    case AST_ASSIGNMENT: {
      _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "process_single", "Executando 'write_identifier'\n");
      if (!__NoxExec_write_identifier(c))
        { _NOX_DEBUG_LOG(
            DEBUG_ERROR,
            "EXECUTOR", 
            "process_single", 
            "Erro ao executar 'write_identifier'\n"
          ); 
          return __NoxBool_FALSE_VALUE; }
    } break;

    case AST_IDENTIFIER: {  
      _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "process_single", "Executando 'read_value'\n");  
      if (!__NoxExec_read_value(c))  
        { _NOX_DEBUG_LOG(
            DEBUG_ERROR,
            "EXECUTOR", 
            "process_single", 
            "Erro ao executar 'read_value'\n"
          );  
          return __NoxBool_FALSE_VALUE; }  
    } break;  

    case AST_BINARY: {  
      _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "process_single", "Executando 'calculate'\n");  
      if (__NoxExec_calculate(c).status != CALC_OK)  
        { _NOX_DEBUG_LOG(
            DEBUG_ERROR,
            "EXECUTOR", 
            "process_single", 
            "Erro ao executar 'calculate'\n"
          );   
          return __NoxBool_FALSE_VALUE; }  
    } break;  

    default: {  
      _NOX_DEBUG_LOG(DEBUG_ERROR, "EXECUTOR", "process_single", "Tipo da AST desconhecido\n");  
      return __NoxBool_FALSE_VALUE;   
    }
  }

  c->index = index;
  c->node = node;
  _NOX_DEBUG_LOG(DEBUG_INFO, "EXECUTOR", "process_single", "Execução concluída\n");

  return __NoxBool_TRUE_VALUE;
}



static inline __NoxValue_BOOLEAN_TYPE
  __NoxExec_process_all(__NoxExec_context* c) 
{
  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "EXECUTOR",
    "process_all",
    "Iniciado\n"
  );

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "EXECUTOR",
    "process_all",
    "Entrando no loop principal\n"
  );

  while (c->index < c->program->statements->count) {
    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "EXECUTOR",
      "process_all",
      "executando 'process_single'\n"
    );
    if (!__NoxExec_process_single(c, c->index)) {
      _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "EXECUTOR",
        "process_all",
        "'process_single' retornou FALSE\n"
      );
      return __NoxBool_FALSE_VALUE;
    }

    c->index++;
  }

  return __NoxBool_TRUE_VALUE;
}