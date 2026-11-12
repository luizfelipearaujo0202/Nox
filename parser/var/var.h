#pragma once

// parser/var/var.h

#include "../../structures/structures.h"
#include <stdlib.h>

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
  __NoxVar_value* value;
  struct __NoxVar__* next;
} __NoxVar__;


static inline __NoxVar_value*
  __NoxVar_value_create(
    __NoxVar_value_type type,
    __NoxVar_v value
    ) {
    __NoxVar_value* v = (__NoxVar_value*) \
      malloc(sizeof(__NoxVar_value));
    if (v == NULL)
      { return NULL; }

    v->_t = type;
    v->_v = value;
    v->ref_count = 1;

    return v;
  }

static inline void
  __NoxVar_value_clear
    (__NoxVar_value* v) {
    if (v == NULL)
      { return; }

    if (v->_t == STRING_VALUE) {
      free(v->_v.v_string);
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

  if (--v->ref_count > 0) 
    { return; }

  __NoxVar_value_clear(v);
  free(v);
}