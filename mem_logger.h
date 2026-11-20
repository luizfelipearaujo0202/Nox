#pragma once

/*
 * mem_logger.h – Logger de alocações de memória para o Nox
 * ------------------------------------------------------
 * Refatorado para corrigir stack‑use‑after‑scope, double‑free e
 * melhorar a segurança do logger.
 */

#include "globals.h"

#ifndef _NOX_MEM_LOGGER_INTERNAL
#  define _NOX_MEM_LOGGER_INTERNAL 1
#endif

#include <stdlib.h>             /* malloc / free */
#include "structures/structures.h" /* __NoxMem_record   */
#include "dynamic_array.h"
#include <pthread.h>               /* mutex */

/* ----------------------------------------------------------------------
 *  Contexto global do logger (mantém o mesmo nome e tipo)
 * ---------------------------------------------------------------------- */
static __NoxMem_context* _NOX_MEM_CTX = NULL;   /* ponteiro inicializado como NULL */
static pthread_mutex_t _NOX_MEM_MTX = PTHREAD_MUTEX_INITIALIZER;

/* ----------------------------------------------------------------------
 *  Autopush – registra a alocação/realloc/free
 *  Agora aloca o registro na heap para evitar stack‑use‑after‑scope.
 * ---------------------------------------------------------------------- */
static inline __NoxValue_BOOLEAN_TYPE
__NoxMem_autopush(
    void*  ptr,
    int    size,
    const char* caller,
    const char* func,
    int    reg
) {
    if (!reg) return __NoxBool_TRUE_VALUE;   /* nada a registrar */

    if (_NOX_MEM_CTX == NULL || _NOX_MEM_CTX->data == NULL)
      { return __NoxBool_FALSE_VALUE; }

    /* aloca registro em heap; será liberado pelo destructor do dynamic array */
    __NoxMem_record* r = (__NoxMem_record*)malloc(sizeof(__NoxMem_record));
    if (!r) return __NoxBool_FALSE_VALUE;

    r->ptr    = ptr;
    r->size   = size;
    r->caller = caller;
    r->func   = func;
    r->freed  = 0;   /* ainda não liberado */

    pthread_mutex_lock(&_NOX_MEM_MTX);
    __NoxValue_BOOLEAN_TYPE ok = __NoxDynaArr_push(
        _NOX_MEM_CTX->data,
        &r,                     /* armazenamos o ponteiro para o registro */
        sizeof(__NoxMem_record*)
    );
    pthread_mutex_unlock(&_NOX_MEM_MTX);

    if (!ok) {
        free(r);               /* falha ao inserir – limpa memória */
        return __NoxBool_FALSE_VALUE;
    }
    return __NoxBool_TRUE_VALUE;
}

/* ----------------------------------------------------------------------
 *  FREE – registra liberação e marca registro como freed
 * ---------------------------------------------------------------------- */
static inline void
__NoxMem_free(
    void*  ptr,
    const char* caller,
    int    reg
) {
    _NOX_DEBUG_LOG(
        DEBUG_MEMORY,
        "MEM",
        "free",
        "liberando %p | %s\n",
        ptr, caller
    );

    /* Guard against uninitialized logger */
    if (!_NOX_MEM_CTX || !_NOX_MEM_CTX->data) {
        free(ptr);
        return;
    }

    if (reg && ptr) {
        pthread_mutex_lock(&_NOX_MEM_MTX);
        size_t count = _NOX_MEM_CTX->data->count;
        for (size_t i = 0; i < count; ++i) {
            __NoxMem_record* const* pp = ( __NoxMem_record* const*)
                __NoxDynaArr_get(_NOX_MEM_CTX->data, i);
            if (pp && *pp && (*pp)->ptr == ptr) {
                (*pp)->freed = 1;
                break;
            }
        }
        pthread_mutex_unlock(&_NOX_MEM_MTX);
    }

    free(ptr);
}


static inline void*
__NoxMem_alloc(
    int    size,
    const char* caller,
    int    reg
) {
  _NOX_DEBUG_LOG(
    DEBUG_MEMORY,
    "MEM",
    "alloc",
    "alocando %d bytes | %s\n",
    size, caller
  );

  void* ptr = malloc(size);
  if (ptr == NULL) return NULL;

  if (!__NoxMem_autopush(
      ptr,
      size,
      caller,
      __func__,
      reg
    ))
  {
    _NOX_FREE_NOREGISTRY(ptr);
    return NULL;
  }

  return ptr;
}


static inline void*
__NoxMem_realloc(
  void*  ptr,
  int    size,
  const char* caller,
  int    reg
) {
  _NOX_DEBUG_LOG(
    DEBUG_MEMORY,
    "MEM",
    "realloc",
    "realocando %d bytes em %p | %s\n",
    size, ptr, caller
  );

  if (ptr == NULL)
    { return __NoxMem_alloc(size, caller, reg); }

  if (size <= 0)
    { return NULL; }

  void* new_ptr = realloc(ptr, size);
  
  if (new_ptr == NULL)
    { return NULL; }

  if (reg && _NOX_MEM_CTX && _NOX_MEM_CTX->data) {
    int found = 0;
    pthread_mutex_lock(&_NOX_MEM_MTX);

    for (
      size_t i = 0;
      i < _NOX_MEM_CTX->data->count;
      i++
    ) {
      __NoxMem_record** pp =
        (__NoxMem_record**)
        __NoxDynaArr_get(_NOX_MEM_CTX->data, i);

      if (pp && *pp && (*pp)->ptr == ptr) {
        (*pp)->ptr = new_ptr;
        (*pp)->size = size;
        (*pp)->freed = 0;
        found = 1;
        break;
      }
    }

    pthread_mutex_unlock(&_NOX_MEM_MTX);

    if (!found) {
      __NoxMem_autopush(
        new_ptr,
        size,
        caller,
        __func__,
        reg
      );
    }
  }

  return new_ptr;
}



static inline void
__NoxMem_destructor(void* element) {
    if (!element) return;
    __NoxMem_record* r = *(__NoxMem_record**)element;
    if (r) free(r);
}

static inline __NoxValue_BOOLEAN_TYPE
__NoxMem_copier(void* dest, const void* src) {
    if (!src) return __NoxBool_FALSE_VALUE;
    *(__NoxMem_record**)dest = *(__NoxMem_record**)src;
    return __NoxBool_TRUE_VALUE;
}


static inline int
__NoxMem_init(void) {
    __NoxDynaArr__* arr = __NoxDynaArr_create(
        sizeof(__NoxMem_record*),   /* armazenamos ponteiros */
        __NoxMem_copier,
        __NoxMem_destructor,
        NULL
    );

    if (!arr) return 0;

    static __NoxMem_context context = {0};
    _NOX_MEM_CTX = &context;
    _NOX_MEM_CTX->data = arr;
    _NOX_MEM_CTX->initialized = 1;
    return 1;
}

/* ----------------------------------------------------------------------
 *  Destroy – libera a estrutura dinâmica (não libera os ponteiros do usuário)
 * ---------------------------------------------------------------------- */
static inline void
__NoxMem_destroy(void) {
    if (!_NOX_MEM_CTX) return;
    if (!__NoxDynaArr_free(_NOX_MEM_CTX->data)) {
        _NOX_DEBUG_LOG(
            DEBUG_ERROR,
            "MEM",
            "destroy",
            "não foi possível liberar *-data\n"
        );
    }
}

/* ----------------------------------------------------------------------
 *  Verify – imprime somente entradas que ainda não foram liberadas
 * ---------------------------------------------------------------------- */
static inline void
__NoxMem_verify(void) {
    if (!_NOX_MEM_CTX || !_NOX_MEM_CTX->initialized) {
        _NOX_DEBUG_LOG(
            DEBUG_ERROR,
            "MEM",
            "verify",
            "contexto de memória não inicializado\n"
        );
        return;
    }

    if (!__NoxDynaArr_is_valid(_NOX_MEM_CTX->data)) {
        _NOX_DEBUG_LOG(
            DEBUG_ERROR,
            "MEM",
            "verify",
            "data do contexto inválida\n"
        );
        return;
    }

    if (_NOX_MEM_CTX->data->count == 0) {
        _NOX_DEBUG_LOG(
            DEBUG_INFO,
            "MEM",
            "verify",
            "array vazio\n"
        );
        return;
    }

    pthread_mutex_lock(&_NOX_MEM_MTX);
    for (int i = 0; i < (int)_NOX_MEM_CTX->data->count; ++i) {
        __NoxMem_record* const* pp = ( __NoxMem_record* const*)
            __NoxDynaArr_get(_NOX_MEM_CTX->data, i);
        if (!pp || !*pp) continue;
        const __NoxMem_record* r = *pp;
        if (r->freed) continue;   /* já liberado */

        _NOX_DEBUG_LOG(
            DEBUG_MEMORY,
            "MEM",
            "verify",
            "--- {POSSÍVEL VAZAMENTO} ---\n"
        );
        _NOX_DEBUG_LOG(
            DEBUG_MEMORY,
            "MEM",
            "verify",
            "Caller: '%s' | Função chamada: '%s'\n",
            r->caller,
            r->func
        );
        _NOX_DEBUG_LOG(
            DEBUG_MEMORY,
            "MEM",
            "verify",
            "Ponteiro vazado: %p\n",
            r->ptr
        );
        _NOX_DEBUG_LOG(
            DEBUG_MEMORY,
            "MEM",
            "verify",
            "Tamanho da memória alocada: %d bytes\n",
            r->size
        );
        _NOX_DEBUG_LOG(
            DEBUG_MEMORY,
            "MEM",
            "verify",
            "----------------------------\n"
        );
    }
    pthread_mutex_unlock(&_NOX_MEM_MTX);
}

/* ----------------------------------------------------------------------
 *  Macros de conveniência – substituem malloc / free / realloc no código‑base
 * ---------------------------------------------------------------------- */
#define NoxMalloc(sz)        __NoxMem_alloc((sz), __FILE__, 1)
#define NoxFree(p)           __NoxMem_free((p), __FILE__, 1)
#define NoxRealloc(p,sz)     __NoxMem_realloc((p), (sz), __FILE__, 1)

/* ----------------------------------------------------------------------
 *  Uso típico (exemplo no main.c):
 *      __NoxMem_init();
 *      atexit(__NoxMem_verify);   // imprime heap restante antes de sair
 *      atexit(__NoxMem_destroy);  // limpa a estrutura interna
 * ---------------------------------------------------------------------- */
