#pragma once

// error/error_renderer.h

#include <stdio.h>

#include "error_aux.h"
#include "../structures/structures.h"

static inline __NoxValue_BOOLEAN_TYPE
  __NoxError_render_line(__NoxError__* e, __NoxError_source* s)
  {
    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "RENDERER",
      "render_line",
      "VALID: %d\n",
        __NoxError_is_valid(e, __NoxBool_TRUE_VALUE)
    );

    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "RENDERER",
      "render_line",
      "SOURCE: %d\n",
        __NoxError_is_source_valid(s)
    );

    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "RENDERER",
      "render_line",
      "POSITION: %d\n",
      __NoxError_is_pos_valid(
        e, s->line->length
      )
    );
    
    if (!__NoxError_is_valid(e, __NoxBool_TRUE_VALUE) ||
        !__NoxError_is_source_valid(s) ||
        !__NoxError_is_pos_valid(e, s->line->length))
      { return __NoxBool_FALSE_VALUE; }

    char* line_src = (char*)malloc(s->line->length + 1);
    if (line_src == NULL)
      { return __NoxBool_FALSE_VALUE; }

    
    char prefix[32];
    int prefix_len = snprintf(prefix, sizeof(prefix), "%d | ", e->line);
    if (prefix_len < 0)
      { free(line_src);
        return __NoxBool_FALSE_VALUE; }

    size_t j = 0;
    for ( size_t i = s->line->start; 
          i < s->line->length + s->line->start; 
          i++) {
      line_src[j] = s->src[i];
      j++;
    }
    line_src[j] = '\0';

    printf("%s: %s\n", __NoxError_get_type(e), e->msg);
    printf("%s%s\n", prefix, line_src);
    free(line_src);

    int i = 0;
    while (i < prefix_len + e->column_start - 1)
      { printf(" "); i++; }

    i = e->column_start + 1;
    printf("^");
    if (e->column_end == 0) {
      while (i <= (int)s->line->length)
        { printf("^"); i++; }
    } else {
      while (i - 1 < e->column_end - 1)
        { printf("^"); i++; }
    }

    printf("\n");

    return __NoxBool_TRUE_VALUE;
  }