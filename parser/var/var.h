#pragma once

// parser/var/var.h

#include "../../structures/structures.h"
#include "../../mem_logger.h"

  typedef
enum {
NONE_VALUE,
NULL_VALUE,
INT_VALUE,
FLOAT_VALUE,
STRING_VALUE,
BOOL_VALUE
} __NoxVar_value_type;

  typedef
union {
  long v_int;
  double v_float;
  char* v_string;
  __NoxValue_BOOLEAN_TYPE v_bool;
  __NoxValue_BOOLEAN_TYPE v_none;
  __NoxValue_BOOLEAN_TYPE v_null;
} __NoxVar_v;

  typedef
struct {
  __NoxVar_value_type _t;
  size_t ref_count;
  __NoxVar_v _v;
} __NoxVar_value;

  typedef
struct __NoxVar__ {
  char* name;

  __NoxValue_BOOLEAN_TYPE explicit_type;
  __NoxVar_value_type declared_type;
  __NoxVar_value* value;

  __NoxToken__* token;
  struct __NoxVar__* next;
} __NoxVar__;


static inline __NoxVar_value*
  __NoxVar_value_create(
    __NoxVar_value_type type,
    __NoxVar_v value
    ) {
    __NoxVar_value* v = (__NoxVar_value*) \
      _NOX_ALLOC(sizeof(__NoxVar_value));
    if (v == NULL)
      { return NULL; }

    v->_t = type;
    v->ref_count = 1;

    // Deep copy pra strings
    if (type == STRING_VALUE && value.v_string != NULL) {
        v->_v.v_string = strdup(value.v_string);
        if (v->_v.v_string == NULL) {
            _NOX_FREE(v);
            return NULL;
        }
    } else {
        v->_v = value;
    }

    return v;
  }

static inline void
  __NoxVar_value_clear
    (__NoxVar_value* v) {
    if (v == NULL)
      { return; }

    if (v->_t == STRING_VALUE) {
      _NOX_FREE(v->_v.v_string);
      v->_v.v_string = NULL;
      return;
    }
    __NoxVar_v _v = {0};
    v->_v = _v; 
  }

static inline void
  __NoxVar_value_release(
    __NoxVar_value* v
  ) {
  if (v == NULL)
    { return; }

  if (--v->ref_count > 1) 
    { return; }

  __NoxVar_value_clear(v);
  _NOX_FREE(v);
}


static inline __NoxValue_BOOLEAN_TYPE
  __NoxVar_is_valid
    (__NoxVar__* n)
{
  if (n == NULL)
    { return __NoxBool_FALSE_VALUE; }

  else if (
    n->name == NULL ||
    n->value == NULL
  ) { return __NoxBool_FALSE_VALUE;}

  return __NoxBool_TRUE_VALUE;
}


static inline __NoxValue_BOOLEAN_TYPE
  __NoxVar_free(
    __NoxVar__* node
  ) {
  if (node == NULL)
    { return __NoxBool_FALSE_VALUE; }

  if (node->value != NULL) 
    {__NoxVar_value_release(node->value);}
  _NOX_FREE(node->name);

  if (node->next != NULL) {
    __NoxVar_free(node->next);
  }

  _NOX_FREE(node);
  return __NoxBool_TRUE_VALUE;
}

static inline void  
  __NoxVar_free_single(
    __NoxVar__* node
  ) {
  if (node == NULL)
    { return; }

  __NoxVar_value_release(node->value);
  _NOX_FREE(node->name);
  _NOX_FREE(node);
}


static inline __NoxVar__*
  __NoxVar_create(
    const char* name,
    const __NoxVar_value* value,
    const __NoxValue_BOOLEAN_TYPE explicit_type,
    const __NoxVar_value_type decl_type,
    const __NoxToken__* token,
    __NoxVar__* next
  )
{
  if (token == NULL)
    { return NULL; }

  if (name == NULL ||
      value == NULL ||
      token->start == NULL)
    { return NULL; }

  __NoxVar__* node =
    (__NoxVar__*)_NOX_ALLOC(
      sizeof(__NoxVar__)
    );
  if (node == NULL)
    { _NOX_FREE(node);
      return NULL; }

  node->name = (char*)_NOX_ALLOC(strlen(name) + 1);
  if (node->name == NULL)
    { _NOX_FREE(node);
      return NULL; }
  strcpy(node->name, name);

  node->value = 
    __NoxVar_value_create(value->_t, value->_v);
  if (node->value == NULL)
  {
    __NoxVar_free(node);
    return NULL;
  }
  node->explicit_type = explicit_type;
  node->declared_type = decl_type;
  node->token = (__NoxToken__*)token;
  node->next = next;
  return node;
}