#pragma once

// error.h

#include "error/error_structures.h"
#include "structures/structures.h"
#include "error/error_aux.h"
#include "error/error_renderer.h"
#include "mem_logger.h"
#include <stdio.h>
#include <string.h>


static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_note_copy(void* dest, const void* src)
  {
    if (dest == NULL || src == NULL)
      { return __NoxBool_FALSE_VALUE; }

    *(__NoxError_note**)dest = (__NoxError_note*)src;
    return __NoxBool_TRUE_VALUE;
  }

// Libera a nota apontada pelo ponteiro no slot
static inline void
  __NoxError_note_destructor(void* element) {
    if (element == NULL) return;

    __NoxError_note* n = *(__NoxError_note**)element;
    
    if (n != NULL) {
      _NOX_FREE(n->msg);
      _NOX_FREE(n);
    }
  }

static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_set_message(__NoxError__* e, const char* msg) 
  {
    if (msg == NULL)
      { return __NoxBool_FALSE_VALUE; }

    if (!__NoxError_is_valid(e, __NoxBool_FALSE_VALUE))
      { return __NoxBool_FALSE_VALUE; }

    e->msg = (char*)_NOX_ALLOC(strlen(msg) + 1);
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

static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_free(__NoxError__* e)
  { if (!__NoxError_is_valid(e, __NoxBool_FALSE_VALUE))
      { return __NoxBool_FALSE_VALUE; }
    _NOX_FREE(e->msg);

    if (e->notes != NULL) {
      if (e->notes->data != NULL)
        {__NoxDynaArr_free(e->notes->data);}
      _NOX_FREE(e->notes);
    }
    _NOX_FREE(e);
    return __NoxBool_TRUE_VALUE; }

static inline __NoxError__*
  __NoxError_create(
    __NoxError_TYPE type,
    const char* msg,
    int line,
    int c_start,
    int c_end,
    __NoxError_note* note
  ) {
    if (msg == NULL)
      { return NULL; }

    if (note != NULL && !__NoxError_note_is_valid(note))
      { return NULL; }

    __NoxError__* e =
      (__NoxError__*)_NOX_ALLOC(sizeof(__NoxError__));

    if (e == NULL)
      { return NULL; }

    e->type = type;
    e->msg = NULL;

    if (!__NoxError_set_message(e, msg) ||
        !__NoxError_set_position(e, line,
                  c_start, c_end)
      )
      { _NOX_FREE(e->msg);
        _NOX_FREE(e);
        return NULL; }

    if (note == NULL){
      e->notes = NULL;
      return e;
    }

    e->notes = (__NoxError_note_array*)
      _NOX_ALLOC(sizeof(__NoxError_note_array));

    if (e->notes == NULL) {
      return e;
    }

    e->notes->data = __NoxDynaArr_create(
      sizeof(__NoxError_note*),
      __NoxError_note_copy,
      __NoxError_note_destructor,
      NULL
    );

    if (e->notes->data == NULL) {
      __NoxError_free(e);
      return NULL;
    }

    if (!__NoxDynaArr_push(
      e->notes->data,
      note,
      sizeof(__NoxError_note*)
      )) { __NoxError_free(e);
          return NULL; }

    return e;
  }