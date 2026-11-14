#pragma once

#include "../structures/structures.h"
#include "../error.h"
#include "error_aux.h"
#include "../dynamic_array.h"
#include "error_structures.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// error/error_registry.h

// Dynamic Array Structures ======

// NoxDynamicArray Destructor
static inline void __NoxError_regarray_destructor(void* element)
{
  if (element == NULL)
    { return; }

  __NoxError_registration* e =
    (__NoxError_registration*)element;

  if (e->error == NULL)
    { return; }
  
  free(e->error->msg);
  free(e->error);
}

// NoxDynamicArray Copier
static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_regarray_copy(void* dest, const void* src)
{
  if (dest == NULL || src == NULL)
    { return __NoxBool_FALSE_VALUE; }

  __NoxError_registration* d = // destination
    (__NoxError_registration*)dest;

  __NoxError_registration* e = // element
    (__NoxError_registration*)src;

  if (e->error == NULL)
    { return __NoxBool_FALSE_VALUE; }

  if (e->error->msg == NULL)
    { return __NoxBool_FALSE_VALUE; }

  d->id = e->id;
  d->fatal = e->fatal;

  d->error = __NoxError_create(
    e->error->type,
    e->error->msg,
    e->error->line,
    e->error->column_start,
    e->error->column_end
  );

  if (d->error == NULL)
    { return __NoxBool_FALSE_VALUE; }

  return __NoxBool_TRUE_VALUE;
}

// NoxDynamicArray Comparator Criteria
// parameter 'config' defined in 'structures.h'

static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_regarray_criteria
    ( const __NoxError_registration* r,
      const __NoxError_regarray_find_config* config)
  {
    if (r == NULL || 
        config == NULL)
    { return __NoxBool_FALSE_VALUE; }
    if (!__NoxError_is_valid(r->error,
            __NoxBool_FALSE_VALUE)) {
      return __NoxBool_FALSE_VALUE;
    }

    switch (config->mode) {
      case NOX_REG_FIND_FATAL: {
        return (r->fatal == config->expected.fatal)
          ? __NoxBool_TRUE_VALUE
          : __NoxBool_FALSE_VALUE;
      }

      case NOX_REG_FIND_ID: {
        return (r->id == config->expected.id)
          ? __NoxBool_TRUE_VALUE
          : __NoxBool_FALSE_VALUE;
      }

      case NOX_REG_FIND_MSG: {
        if (r->error->msg == NULL ||
            config->expected.msg == NULL)
          { return __NoxBool_FALSE_VALUE; }
        return strcmp(r->error->msg,config->expected.msg)==0
          ? __NoxBool_TRUE_VALUE
          : __NoxBool_FALSE_VALUE;
      }

      case NOX_REG_FIND_TYPE: {
        return (r->error->type == config->expected.type)
          ? __NoxBool_TRUE_VALUE
          : __NoxBool_FALSE_VALUE;
      }

      default: return __NoxBool_FALSE_VALUE;
    }
  }

// NoxDynamicArray Comparator
/* 
'criteria' param is a struct, defined
in 'structures.h' named:
'__NoxError_regarray_find_config'
*/ 
static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_regarray_comparator
    (const void* element, const void* criteria)
  {
    if (element == NULL ||
        criteria == NULL)
      { return __NoxBool_FALSE_VALUE; }

    __NoxError_registration* e =
      (__NoxError_registration*)element;

    __NoxError_regarray_find_config* c =
    (__NoxError_regarray_find_config*)criteria;

    return \
      __NoxError_regarray_criteria(e, c);
  }
// ===============================

static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_regarray_is_valid
    (__NoxError_regarray* arr)
  { if (arr == NULL || arr->data == NULL)
      { return __NoxBool_FALSE_VALUE; }
    return __NoxDynaArr_is_valid(arr->data); }

static inline int
  __NoxError_get_index
    (__NoxError_regarray* arr,
    __NoxError_registration* e) {
  if (!__NoxError_regarray_is_valid(arr))
    { return -1; }
  if (e == NULL)
    { return -1; }
  else if (
    !__NoxError_is_valid(e->error, __NoxBool_FALSE_VALUE)
  ) { return -1; }

  __NoxError_regarray_find_config c = {
    .mode = NOX_REG_FIND_ID,
    .expected = {
      .id = e->id
    }
  };

  int index = __NoxDynaArr_find(arr->data, &c);
  return index;
}


static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_regarray_clear
    (__NoxError_regarray* arr) {
    if (!__NoxError_regarray_is_valid(arr) ||
        !__NoxDynaArr_free(arr->data))
      { return __NoxBool_FALSE_VALUE; }

    arr->data = NULL;
    arr->next_id = 0;
    return __NoxBool_TRUE_VALUE;
  }


static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_regarray_free
    (__NoxError_regarray* arr) {
    if (!__NoxError_regarray_clear(arr))
      { return __NoxBool_FALSE_VALUE; }

    free(arr);
    return __NoxBool_TRUE_VALUE;
  }


static inline __NoxError_regarray*
  __NoxError_create_regarray()
  {
    __NoxError_regarray* arr = 
      (__NoxError_regarray*) \
      malloc(sizeof(__NoxError_regarray));

    if (arr == NULL) 
      { return NULL; }

    arr->data = __NoxDynaArr_create(
      sizeof(__NoxError_registration),
      __NoxError_regarray_copy,
      __NoxError_regarray_destructor,
      __NoxError_regarray_comparator
    );

    if (arr->data == NULL)
      { free(arr);
        return NULL; }

    arr->next_id = 0;

    return arr;
  }


static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_register(
    __NoxError_regarray* arr,
    __NoxError__* error,
    __NoxValue_BOOLEAN_TYPE fatal
  ) {
    if (!__NoxError_regarray_is_valid(arr) ||
        !__NoxError_is_valid(error, __NoxBool_FALSE_VALUE))
    { return __NoxBool_FALSE_VALUE; }

    __NoxError_registration reg;
    reg.fatal = fatal;
    reg.error = error;
    reg.id = arr->next_id + 1;

    if (!__NoxDynaArr_push(
        arr->data,
        &reg,
        sizeof(reg)
      ))
      { return __NoxBool_FALSE_VALUE; }

    arr->next_id++;
    return __NoxBool_TRUE_VALUE;
  }


static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_regarray_remove(
    __NoxError_regarray* arr,
    __NoxError_registration* e
  ) {
  if (!__NoxError_regarray_is_valid(arr) ||
      e == NULL)
  { return __NoxBool_FALSE_VALUE; }

  if (!__NoxError_is_valid(e->error, __NoxBool_FALSE_VALUE))
  { return __NoxBool_FALSE_VALUE; }

  int index = __NoxError_get_index(arr, e);

  if (index == -1)
    { return __NoxBool_FALSE_VALUE; }

  return __NoxDynaArr_remove(arr->data, index);
}


static inline __NoxError_registration*
  __NoxError_get_first
    (__NoxError_regarray* arr) {
    if (!__NoxError_regarray_is_valid(arr))
      { return NULL; }
    else return (__NoxError_registration*) \
      __NoxDynaArr_get(arr->data, 0);
  }

static inline __NoxError_registration*
  __NoxError_get_last
    (__NoxError_regarray* arr) {
    if (!__NoxError_regarray_is_valid(arr))
      { return NULL; }
    else return (__NoxError_registration*) \
      __NoxDynaArr_get(arr->data, arr->data->count - 1);
  }

static inline __NoxError_registration*
  __NoxError_get_first_fatal
    (__NoxError_regarray* arr) {
    if (!__NoxError_regarray_is_valid(arr))
      { return NULL; }

    __NoxError_regarray_find_config c = {
      .mode = NOX_REG_FIND_FATAL,
      .expected = {
        .fatal = __NoxBool_TRUE_VALUE,
      }
    };

    int index = __NoxDynaArr_find(arr->data, &c);
    if (index != -1)
      { return (__NoxError_registration*)\
        __NoxDynaArr_get(arr->data, index); }
    return NULL;
  }

static inline __NoxError_registration*
  __NoxError_get_last_fatal
  (__NoxError_regarray* arr) {
    if (!__NoxError_regarray_is_valid(arr))
      { return NULL; }

    __NoxError_regarray_find_config c = {
      .mode = NOX_REG_FIND_FATAL,
      .expected = {
        .fatal = __NoxBool_TRUE_VALUE,
      }
    };

    int index = __NoxDynaArr_find_last(arr->data, &c);
    if (index != -1)
      { return (__NoxError_registration*)\
        __NoxDynaArr_get(arr->data, index); }
    return NULL;
  }