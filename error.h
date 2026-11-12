#pragma once

// error.h

#include "structures/structures.h"
#include "error/error_aux.h"
#include "error/error_renderer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_set_message(__NoxError__* e, const char* msg) 
  {
    if (msg == NULL)
      { return __NoxBool_FALSE_VALUE; }

    if (!__NoxError_is_valid(e, __NoxBool_FALSE_VALUE))
      { return __NoxBool_FALSE_VALUE; }

    e->msg = (char*)malloc(strlen(msg) + 1);
    if (e->msg == NULL)
      { return __NoxBool_FALSE_VALUE; }
    strcpy(e->msg, msg);
    return __NoxBool_TRUE_VALUE;
  }

static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_set_position(
      __NoxError__* e, 
      int line,
      int column_start,
      int column_end
    ) {
    if (!__NoxError_is_valid(e, __NoxBool_TRUE_VALUE) || 
        line <= 0 ||
        column_start <= 0 || column_end < 0) 
    { return __NoxBool_FALSE_VALUE; }

    e->line = line;

    e->column_start = column_start;
    e->column_end = column_end;
    return __NoxBool_TRUE_VALUE;
  }

static inline __NoxError__*
  __NoxError_create(
    __NoxError_TYPE type,
    const char* msg,
    int line,
    int c_start,
    int c_end
  ) {
    if (msg == NULL)
      { return NULL; }

    __NoxError__* e =
      (__NoxError__*)malloc(sizeof(__NoxError__));

    if (e == NULL)
      { return NULL; }

    e->type = type;
    e->msg = NULL;

    if (!__NoxError_set_message(e, msg) ||
        !__NoxError_set_position(e, line,
                  c_start, c_end)
      )
      { free(e->msg);
        free(e);
        return NULL; }

    return e;
  }

static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_free(__NoxError__* e)
  { if (!__NoxError_is_valid(e, __NoxBool_FALSE_VALUE))
      { return __NoxBool_FALSE_VALUE; }
    free(e->msg);
    free(e);
    return __NoxBool_TRUE_VALUE; }