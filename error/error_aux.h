#pragma once

// error_aux.h

#include "../structures/structures.h"
#include "error_structures.h"
#include <stdlib.h>

static inline __NoxError_source_line* 
  __NoxError_get_source_line
      ( const char* src, 
        int line) {
    if (src == NULL || 
        line <= 0)
      { return NULL; }

    __NoxError_source_line* sl =
      (__NoxError_source_line*)malloc
        (sizeof(__NoxError_source_line));

    if (sl == NULL) { return NULL; }

    sl->length = 0;
    sl->start = 0;

    __NoxValue_BOOLEAN_TYPE found =
      __NoxBool_FALSE_VALUE;
    size_t i = 0;
    int ln = 1;
    for (; src[i] != '\0'; i++) {
      if (src[i] == '\n') 
        { ln++;
          continue; }

      else if (ln == line)
      {
        found = __NoxBool_TRUE_VALUE;
        size_t j = 0;
        sl->start = i;
        while (src[i+j] != '\n' && src[i+j] != '\0') 
        { j++; }

        sl->length = j;
        break;
      }
    }
    if (!found)
      { free(sl); return NULL; }
    return sl;
  }

static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_is_source_valid(__NoxError_source* s)
  { if (s != NULL && 
        s->src != NULL &&
        s->line != NULL)
      { return __NoxBool_TRUE_VALUE; }
    return __NoxBool_FALSE_VALUE; }

static inline __NoxError_source*
  __NoxError_create_source(const char* src, int line)
    { if (src == NULL) return NULL;

      __NoxError_source_line* sl =
        __NoxError_get_source_line(src, line);
      if (sl == NULL) return NULL;

      __NoxError_source* s =
        (__NoxError_source*)malloc(sizeof(__NoxError_source));
      if (s == NULL) 
        { free(sl);
          return NULL; }

      s->src = src;
      s->line = sl;
      return s;
    }

static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_free_source(__NoxError_source* s)
    { if (s == NULL) return __NoxBool_FALSE_VALUE;
      free(s->line);
      free(s);
      return __NoxBool_TRUE_VALUE; }



static inline const char*
  __NoxError_get_type(const __NoxError__* e)
  {
    if (e == NULL) return NULL;
    switch (e->type) {
      case ERROR_LEXER: return "LexicalError";
      case ERROR_SYNTAX: return "SyntaxError";
      case ERROR_SEMANTIC: return "SemanticError";
      case ERROR_TYPE: return "TypeError";
      case ERROR_NAME: return "NameError";
      case ERROR_RUNTIME: return "RuntimeError";
    }
    return "";
  }

static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_is_valid(
      const __NoxError__* e,
      __NoxValue_BOOLEAN_TYPE msg
    ) { if (e != NULL && (!msg || e->msg != NULL))
        { return __NoxBool_TRUE_VALUE; }
      return __NoxBool_FALSE_VALUE; }

static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_is_pos_valid
        ( const __NoxError__* e,
          int line_len)
  { if (!__NoxError_is_valid(e, __NoxBool_TRUE_VALUE) ||
        line_len <= 0)
      { return __NoxBool_FALSE_VALUE; }

    else if (e->line > 0 &&
        e->column_start > 0) 
      {
        if ((e->column_end <= line_len + 1 &&
            e->column_end == 0) ||
            ( e->column_end > 0 &&
              e->column_start <= e->column_end)
        ) 
        { return __NoxBool_TRUE_VALUE; }
    }
    return __NoxBool_FALSE_VALUE; }
