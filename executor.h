#pragma once

#include "debug.h"

#include "error.h"
#include "error/error_aux.h"
#include "error/error_registry.h"
#include "error/error_structures.h"

#include "parser/var/var.h"
#include "parser/var/var_table.h"

#include "structures/ast_structures.h"
#include "structures/exec_structures.h"
#include "structures/structures.h"

#include "dynamic_array.h"

#include "mem_logger.h"

#include <errno.h>
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

static inline __NoxValue_BOOLEAN_TYPE
__NoxExec_create_error(
    __NoxExec_context* c,
    __NoxError_TYPE type,
    __NoxError_note* note,
    int line,
    int c_start,
    int c_end,
    const char* format,
    ...
) {
  if (!__NoxExec_context_is_valid(c))
    { return __NoxBool_FALSE_VALUE; }
  if (
    c->errors == NULL ||
    c->errors->registry == NULL ||
    format == NULL
  )
    { return __NoxBool_FALSE_VALUE; }

  if (note != NULL && !__NoxError_note_is_valid(note))
    { return __NoxBool_FALSE_VALUE; }

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
    c_end,
    note
  );
  if (e == NULL)
    { return __NoxBool_FALSE_VALUE; }

  __NoxError_register(
    c->errors->registry,
    e,
    __NoxBool_TRUE_VALUE
  );

  __NoxError_free(e);

  return __NoxBool_TRUE_VALUE;
}

// =======================

// forward declaration to eval_operand function
static inline __NoxExec_CALC_RESULT
  __NoxExec_calculate(__NoxExec_context* c);

static inline __NoxExec_CALC_STATUS
  __NoxExec_eval_operand(
    __NoxExec_context* c,
    __NoxAST_NODE* operand,
    long* out_val
  ) {
  if (operand == NULL) return CALC_INVALID_ACCESS;

  if (operand->_type == AST_BINARY) {
      __NoxExec_context sub_ctx = *c;
      sub_ctx.node = operand;
      __NoxExec_CALC_RESULT res = __NoxExec_calculate(&sub_ctx);
      if (res.status != CALC_OK) return res.status;
      *out_val = res.result;
      return CALC_OK;
  }

  if (operand->_type == AST_LITERAL) {
      if (operand->data.token->_literal_type != VALUE_INT_LITERAL) {
          return CALC_A_NOT_NUMBER;
      }
      *out_val = strtol(operand->data.token->start, NULL, 10);
      return CALC_OK;
  }

  if (operand->_type == AST_IDENTIFIER) {
      __NoxToken__* t = operand->data.token;
      char key[t->length + 1];
      memcpy(key, t->start, t->length);
      key[t->length] = '\0';

      __NoxVar__* var = __NoxVar_HashTable_get(c->table, key);
      if (var == NULL) {
          __NoxExec_create_error(c, ERROR_RUNTIME, NULL,
              t->line, t->column, t->column + t->length,
              "variable '%s' not declared", key);
          return CALC_A_NOT_NUMBER;
      }
      if (var->value == NULL || var->value->_t != INT_VALUE) {
          __NoxExec_create_error(c, ERROR_TYPE, NULL,
              t->line, t->column, t->column + t->length,
              "variable '%s' is not an integer", key);
          return CALC_A_NOT_NUMBER;
      }
      *out_val = var->value->_v.v_int;
      return CALC_OK;
  }
  return CALC_INVALID_AST_TYPE;
}


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

  _NOX_DEBUG_LOG(DEBUG_VERIFY, "EXECUTOR", "calculate", "side's type:\n  Left: %s\n  Right: %s\n",
    __NoxDebug_show_ast_type(node->data.binary.left),
    __NoxDebug_show_ast_type(node->data.binary.right)
  );

  long left = 0;
  long right = 0;

  __NoxExec_CALC_STATUS st_left = 
    __NoxExec_eval_operand(c, node->data.binary.left, &left);
  if (st_left != CALC_OK) {
      r.status = st_left;
      return r;
  }

  __NoxExec_CALC_STATUS st_right =
    __NoxExec_eval_operand(c, node->data.binary.right, &right);
  if (st_right != CALC_OK) {
      r.status = st_right;
      return r;
  }

  long res = 0;

  _NOX_DEBUG_LOG(DEBUG_VERIFY, "EXECUTOR", "calculate", "operator: %s\n", 
    __NoxDebug_show_ast_operator(node));
  
  if (node->data.binary.op == AST_PLUS) 
    { res = left + right; } 
  else if (node->data.binary.op == AST_MINUS)
    { res = left - right; }
  else if (node->data.binary.op == AST_MULTIPLY)
    { res = left * right; }
  else if (node->data.binary.op == AST_DIVIDE)
  { 
    const __NoxToken__* t =
      t = node->data.binary.right->data.token;

    if (left == 0)
      { res = 0; }

    else if (right == 0) {
      __NoxExec_create_error(
        c,
        ERROR_EXPRESSION,
        NULL,
        t->line,
        t->column,
        t->column +
          t->length,
        "division by zero"
      );
      r.status = CALC_DIVISION_BY_ZERO; 
      return r; 
    }
    else 
      { res = left / right; } 
  }
  else if (node->data.binary.op == AST_MODULE)
  {
    const __NoxToken__* t =
      t = node->data.binary.right->data.token;

    if (left == 0)
      { res = 0; }

    else if (right == 0) {
      __NoxExec_create_error(
        c,
        ERROR_EXPRESSION,
        NULL,
        t->line,
        t->column,
        t->column +
          t->length,
        "division by zero"
      );
      r.status = CALC_DIVISION_BY_ZERO; 
      return r; 
    }
    else 
      { res = left % right; } 
  }

  else if (node->data.binary.op == AST_EQUAL)
    { res = (left == right); }
  else if (node->data.binary.op == AST_NOT_EQUAL)
    { res = (left != right); }
  else if (node->data.binary.op == AST_LESS)
    { res = (left < right); }
  else if (node->data.binary.op == AST_GREATER)
    { res = (left > right); }
  else if (node->data.binary.op == AST_LESS_EQUAL)
    { res = (left <= right); }
  else if (node->data.binary.op == AST_GREATER_EQUAL)
    { res = (left >= right); }

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
      return NULL;
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
  char* key = (char*)_NOX_ALLOC(t->length + 1);
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
      if (!__NoxExec_create_error(
        c,
        ERROR_RUNTIME,
        NULL,
        t->line,
        t->column,
        t->column +
          t->length,
        "variable '%.*s' not declared",
        (int)t->length, key
      )) {
        _NOX_DEBUG_LOG(
          DEBUG_VERIFY,
          "",
          "",
          ": ERROR\n"
        );
      }
      else {
        _NOX_DEBUG_LOG(
          DEBUG_VERIFY,
          "",
          "",
          ": OK\n"
        );
      }
    }
    _NOX_FREE(key);
    return NULL;
  }
  _NOX_DEBUG_LOG(DEBUG_VERIFY, "", "", "[DONE]\n");

  _NOX_FREE(key);
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

      return NULL;
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
  char* key = (char*)_NOX_ALLOC(t->length + 1);
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

      if (!__NoxExec_create_error(
        c,
        ERROR_RUNTIME,
        NULL,
        t->line,
        t->column,
        t->column +
          t->length,
        "variable '%.*s' not declared",
        (int)t->length, key
      )) {
        _NOX_DEBUG_LOG(
          DEBUG_VERIFY,
          "", "",
          ": ERROR\n"
        );
      }
      else {
        _NOX_DEBUG_LOG(
          DEBUG_VERIFY,
          "", "",
          ": OK\n"
        );
      }
    }
    _NOX_FREE(key);
    return NULL;
  }
  _NOX_DEBUG_LOG(DEBUG_VERIFY, "", "", "[DONE]\n");

  _NOX_FREE(key);
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

  size_t key_len = node->data.assignment.identifier->data.token->length;
  char* key = (char*)_NOX_ALLOC(key_len + 1);
  if (key == NULL)
    { return __NoxBool_FALSE_VALUE; }
  memcpy(key, node->data.assignment.identifier->data.token->start, key_len);
  key[key_len] = '\0';

  char* value_str = NULL;
  __NoxVar_value* value = NULL;

  // Verifica operações matemáticas vindo do value
  if (node->data.assignment.value->_type == AST_BINARY)
  { c->node = node->data.assignment.value;
    __NoxExec_CALC_RESULT res =
      __NoxExec_calculate(c); 
    c->node = node;

    if (res.status != CALC_OK) {
      _NOX_FREE(key);
      return __NoxBool_FALSE_VALUE;
    }

    value = __NoxVar_value_create(
      INT_VALUE,
      (__NoxVar_v){
        .v_int = res.result
      }
    );
  }
  else if (node->data.assignment.value->data.token == NULL)
    { _NOX_FREE(key);
      return __NoxBool_FALSE_VALUE; }
  else {
    size_t value_len = node->data.assignment.value->data.token->length;
    value_str = (char*)_NOX_ALLOC(value_len + 1);
    if (value_str == NULL)
      { _NOX_FREE(key);
        return __NoxBool_FALSE_VALUE; }
    memcpy(value_str, node->data.assignment.value->data.token->start, value_len);
    value_str[value_len] = '\0';
  }

  /*
  Verifica se uma variável utilizada no valor existe
  */
  __NoxValue_BOOLEAN_TYPE is_shared =
    __NoxBool_FALSE_VALUE; // Verifica se valor é de var existente
  __NoxVar__* existing_var = NULL;
  if (
    node->data.assignment.value->_type == AST_IDENTIFIER || 
    (
      node->data.assignment.value->_type == AST_LITERAL && 
      node->data.assignment.value->data.token->_type == IDENTIFIER
    )
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

        _NOX_DEBUG_LOG(
          DEBUG_INFO,
          "EXECUTOR",
          "write_identifier",
          "criando erro de nome"
        );
        if (!__NoxExec_create_error(
          c,
          ERROR_NAME,
          NULL,
          t->line,
          t->column,
          t->column + t->length,
          "use of undefined identifier '%.*s'",
          (int)t->length, t->start
        )) {
          _NOX_DEBUG_LOG(
            DEBUG_VERIFY,
            "", "",
            ": ERROR\n"
          );
        }
        else {
          _NOX_DEBUG_LOG(
            DEBUG_VERIFY,
            "", "",
            ": OK\n"
          );
        }
      }

      _NOX_FREE(key);
      _NOX_FREE(value_str);
      return __NoxBool_FALSE_VALUE;
    }
  }
  else if (node->data.assignment.value->_type == AST_LITERAL) {
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

  if (value == NULL) {
    _NOX_DEBUG_LOG(
      DEBUG_ERROR,
      "EXECUTOR",
      "write_identifier",
      "'value' == NULL\n"
    );
    _NOX_FREE(key);
    _NOX_FREE(value_str);
    return __NoxBool_FALSE_VALUE;
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
  __NoxValue_BOOLEAN_TYPE explicit_t =
    __NoxBool_FALSE_VALUE;
  __NoxVar_value_type decl_type = NONE_VALUE;
  const __NoxToken__* t = 
    node->data.assignment.identifier->data.token;
  const __NoxToken__* v =
    __NoxAST_get_first_token(node->data.assignment.value);
  
  if (v == NULL) v = t;

  __NoxDebug_show_var(existing);

  if (node->data.assignment.explicit_type) {
    explicit_t = __NoxBool_TRUE_VALUE;
    decl_type = node->data.assignment.type;
  }
  else if (existing != NULL) {
    explicit_t = existing->explicit_type;
    decl_type = existing->declared_type;
  }

  if (
    explicit_t && 
    decl_type != value->_t
  ) {
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
      __NoxDebug_value_type_name(decl_type)
    );

    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "EXECUTOR",
      "write_identifier",
      "criando nota\n"
    );
    __NoxError_note* note = NULL;

    if (existing != NULL && existing->token != NULL) {
      note = __NoxError_create_note(
        "declared before here",
        existing->token->line,
        existing->token->column,
        existing->token->column +
          existing->token->length
      );

      _NOX_DEBUG_LOG(
        DEBUG_VERIFY,
        "EXECUTOR",
        "write_identifier",
        "nota criada: %p\n",
        (void*)note
      );
    }
    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "EXECUTOR",
      "write_identifier",
      "criando erro de runtime"
    );
    if(!__NoxExec_create_error(
      c, ERROR_TYPE, 
      note,
      v->line,
      v->column,
      v->column + v->length,
      "cannot assign type '%s' to fixed variable '%s' of type '%s'",
      __NoxDebug_value_type_name_lower(value->_t),
      key,
      __NoxDebug_value_type_name_lower(decl_type)
    )) {
      _NOX_DEBUG_LOG(
        DEBUG_VERIFY,
        "", "",
        ": ERROR\n"
      );
    }
    else {
      _NOX_DEBUG_LOG(
        DEBUG_VERIFY,
        "", "",
        ": OK\n"
      );
    }

    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "EXECUTOR",
      "write_identifier",
      "erro criado"
    );
    
    // Limpa o value novo que não vai ser usado
    if (!is_shared) {
        __NoxVar_value_clear(value);
        _NOX_FREE(value);
    }

    _NOX_FREE(key);
    _NOX_FREE(value_str);
    return __NoxBool_FALSE_VALUE;
  }
  __NoxVar__* new_var = __NoxVar_create(
    key,
    value,
    explicit_t,
    decl_type,
    t,
    NULL
  );

  if (new_var == NULL) {
      if (!is_shared) { __NoxVar_value_clear(value); _NOX_FREE(value); }
      _NOX_FREE(key); _NOX_FREE(value_str);
      return __NoxBool_FALSE_VALUE;
  }

  __NoxVar_HashTable_insert(c->table, new_var);
  
  __NoxDebug_show_var(new_var);

  __NoxVar_free(new_var);
  
  if (!is_shared) {
      __NoxVar_value_clear(value);
      _NOX_FREE(value);
  }

  _NOX_FREE(value_str);
  _NOX_FREE(key);

  return __NoxBool_TRUE_VALUE;
}


static inline __NoxValue_BOOLEAN_TYPE
  __NoxExec_process_node(
    __NoxExec_context* c,
    __NoxAST_NODE* node
  );

static inline __NoxValue_BOOLEAN_TYPE
__NoxExec_process_statement_array(
  __NoxExec_context* c,
  __NoxDynaArr__* statements
) {
  if (!__NoxExec_context_is_valid(c))
    { return __NoxBool_FALSE_VALUE; }

  if (statements == NULL)
    { return __NoxBool_FALSE_VALUE; }
  if (!__NoxDynaArr_is_valid(statements))
    { return __NoxBool_FALSE_VALUE; }

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "EXECUTOR",
    "process_statement_array",
    "Processing %zu statements\n",
    statements->count
  );

  for (size_t i = 0; i < statements->count; i++) {
    _NOX_DEBUG_LOG(
      DEBUG_VERIFY,
      "EXECUTOR",
      "process_statement_array",
      "Statement %zu/%zu\n",
      i + 1, statements->count
    );

    __NoxAST_NODE* node =
      *(__NoxAST_NODE**)__NoxDynaArr_get(
        statements, i
      );

    if (node == NULL) {
      _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "EXECUTOR",
        "process_statement_array",
        "Statement at index %zu is NULL\n",
        i
      );
      return __NoxBool_FALSE_VALUE;
    }

    if (!__NoxExec_process_node(c, node)) { 
      _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "EXECUTOR",
        "process_statement_array",
        "Failed to execute statement at index %zu\n",
        i
      );
      return __NoxBool_FALSE_VALUE; 
    }
  }

  return __NoxBool_TRUE_VALUE;
}


static inline __NoxValue_BOOLEAN_TYPE
__NoxExec_process_node(
  __NoxExec_context *c,
  __NoxAST_NODE *node
) {
  if (!__NoxExec_context_is_valid(c))
    { return __NoxBool_FALSE_VALUE; }

  if (node == NULL) {
    _NOX_DEBUG_LOG(
      DEBUG_ERROR,
      "EXECUTOR",
      "process_node",
      "Cannot process NULL node\n"
    );
  }

  c->node = node;

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "EXECUTOR",
    "process_node",
    "Processing node: %s\n",
    __NoxDebug_show_ast_type(node)
  );

  switch (node->_type) {
    case AST_ASSIGNMENT:
      return __NoxExec_write_identifier(c);

    case AST_BINARY:
      return
        (__NoxExec_calculate(c).status == CALC_OK)
        ? __NoxBool_TRUE_VALUE
        : __NoxBool_FALSE_VALUE;

    case AST_IF: {
      if (node->data.if_statement.condition == NULL)
        { return __NoxBool_FALSE_VALUE; }

      __NoxExec_context condition_ctx = *c;
      condition_ctx.node =
          node->data.if_statement.condition;

      __NoxExec_CALC_RESULT result =
          __NoxExec_calculate(&condition_ctx);

      if (result.status != CALC_OK)
        { return __NoxBool_FALSE_VALUE; }

      if (result.result != 0) {
        // executa THEN
        return __NoxExec_process_statement_array(
          c,
          node->data.if_statement.then_branch
        );
      }

      if (node->data.if_statement.else_branch != NULL) {
        // Executa ELSE se existir
        return __NoxExec_process_statement_array(
          c,
          node->data.if_statement.else_branch
        );
      }

      return __NoxBool_TRUE_VALUE;
    }

    default: 
      _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "EXECUTOR",
        "process_node",
        "Unknown node: %s",
        __NoxDebug_show_ast_type(node)
      );
      return __NoxBool_FALSE_VALUE;
  }
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

  c->index = index;

  return __NoxExec_process_node(c, node);
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