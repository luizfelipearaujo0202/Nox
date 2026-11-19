#pragma once

// error/error_renderer.h

#include <stdio.h>

#include "error_aux.h"
#include "../structures/structures.h"
#include "error_structures.h"
#include "../mem_logger.h"

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

    char* line_src = (char*)_NOX_ALLOC(s->line->length + 1);
    if (line_src == NULL)
      { return __NoxBool_FALSE_VALUE; }

    
    char prefix[32];
    int prefix_len = snprintf(prefix, sizeof(prefix), "%d | ", e->line);
    if (prefix_len < 0)
      { _NOX_FREE(line_src);
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
    _NOX_FREE(line_src);

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

    // Note rendering
    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "RENDERER",
      "render_line",
      "verificando se existem notas"
    );
    if (e->notes != NULL && e->notes->data->count > 0)
    {
      _NOX_DEBUG_LOG(
        DEBUG_INFO,
        "", "",
        ": EXISTEM\n"
      );

      for (size_t k = 0; k < e->notes->data->count; k++) {
        _NOX_DEBUG_LOG(
          DEBUG_INFO,
          "RENDERER",
          "render_line",
          "índice atual: %zu\n",
          k
        );
        __NoxError_note* note = *(__NoxError_note**)
          __NoxDynaArr_get(e->notes->data, k);
        
        _NOX_DEBUG_LOG(
          DEBUG_VERIFY,
          "RENDERER",
          "render_line",
          "nota: %p\n",
          (void*)note
        );

        if (note == NULL) continue;                               

        printf(" note: %s:\n", note->msg);

        _NOX_DEBUG_LOG(
          DEBUG_INFO,
          "RENDERER",
          "render_line",
          "coletando source_line"
        );

        __NoxError_source_line* note_sl = 
          __NoxError_get_source_line(s->src, note->line); 

        if (note_sl != NULL) {
          _NOX_DEBUG_LOG(
            DEBUG_VERIFY,
            "", "",
            ": OK\n"
          );
          char* note_src = (char*)_NOX_ALLOC(note_sl->length + 1);
          if (note_src != NULL) {
            memcpy(note_src, s->src + note_sl->start, note_sl->length);
            note_src[note_sl->length] = '\0';

            printf(" %d | %s\n", note->line, note_src);
              _NOX_FREE(note_src); 
          }
          _NOX_FREE(note_sl);
        }
        else {
          _NOX_DEBUG_LOG(
            DEBUG_VERIFY,
            "", "",
            ": ERROR\n"
          );
        }
      }
    }
    else {
      _NOX_DEBUG_LOG(
        DEBUG_VERIFY,
        "","",
        ": NÃO EXISTEM"
      );
    }
    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "", "","\n"
    );

    return __NoxBool_TRUE_VALUE;
  }