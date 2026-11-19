#pragma once

// globals.h

#include "structures/structures.h"

/*
====================
== INITIAL VALUES ==
====================
*/

static inline void
  __NoxMem_free(
    void* ptr,
    const char* caller,
    int reg
  );

static inline void*
  __NoxMem_alloc(
    int size,
    const char* caller,
    int reg
  );

static inline void*
  __NoxMem_realloc(
    void* ptr,
    int size,
    const char* caller,
    int reg
  );

#ifndef _NOX_DYNAMIC_ARRAY_ENABLED
#  define _NOX_DYNAMIC_ARRAY_ENABLED 0
#endif

#ifndef _NOX_MEM_LOGGER_INTERNAL
#  define _NOX_MEM_LOGGER_INTERNAL 0
#endif

extern unsigned int _NOX_DEBUG_ENABLED;

// ====================


/*
=====================================
== MEM_LOGGER (mem_logger.h) FLAGS ==
=====================================
*/

// INITIALIZER & DESTROYER
#define _NOX_MEM_INIT() \
  do { \
    if (!__NoxMem_init()) {\
      _NOX_MEM_CTX->initialized = 0; \
      break; \
    } \
    _NOX_MEM_CTX->initialized = 1; \
  } while (0)

#define _NOX_MEM_DESTROY() \
  do { \
    if (!_NOX_MEM_CTX->initialized) { \
      break; \
    } \
    __NoxMem_destroy(); \
    break; \
  } while (0)

#define _NOX_FREE(ptr) \
  __NoxMem_free(ptr, __func__, 1)

#define _NOX_ALLOC(size) \
  __NoxMem_alloc(size, __func__, 1)

#define _NOX_REALLOC(ptr, size) \
  __NoxMem_realloc(ptr, size, __func__, 1)


#define _NOX_FREE_NOREGISTRY(ptr) \
  __NoxMem_free(ptr, __func__, 0)
#define _NOX_ALLOC_NOREGISTRY(size) \
  __NoxMem_alloc(size, __func__, 0)

#define _NOX_REALLOC_NOREGISTRY(ptr, size) \
  __NoxMem_realloc(ptr, size, __func__, 0)

// ==================================



/*
===========================================
== DYNAMIC ARRAY (dynamic_array.h) FLAGS ==
===========================================
*/

#if _NOX_DYNAMIC_ARRAY_ENABLED
#  if _NOX_MEM_LOGGER_INTERNAL
#    define _NOX_DYNA_ALLOC(size) \
      _NOX_ALLOC_NOREGISTRY(size)
#    define _NOX_DYNA_REALLOC(ptr, size) \
      _NOX_REALLOC_NOREGISTRY(ptr, size)
#    define _NOX_DYNA_FREE(ptr) \
      _NOX_FREE_NOREGISTRY(ptr)
#  else
#    define _NOX_DYNA_ALLOC(size) \
      _NOX_ALLOC(size)
#    define _NOX_DYNA_REALLOC(ptr, size) \
      _NOX_REALLOC(ptr, size)
#    define _NOX_DYNA_FREE(ptr) \
      _NOX_FREE(ptr)
#  endif
#else
#    define _NOX_DYNA_ALLOC(size) \
      _NOX_ALLOC_NOREGISTRY(size)
#    define _NOX_DYNA_REALLOC(ptr, size) \
      _NOX_REALLOC_NOREGISTRY(ptr, size)
#    define _NOX_DYNA_FREE(ptr) \
      _NOX_FREE_NOREGISTRY(ptr)
#  endif

// ========================================




/*
============================
== DEBUG FLAGS AND MACROS ==
============================
*/
typedef enum {
  DEBUG_NONE = 0,
  DEBUG_VERIFY = 1 << 0, // 0001
  DEBUG_INFO = 1 << 1, // 0010
  DEBUG_ERROR = 1 << 2, // 0100
  DEBUG_MEMORY = 1 << 3, // 1000
  DEBUG_ALL = 0xFFFF // 1111
} __NoxDebug_LEVEL;

#define _NOX_DEBUG_LOG(level, module, function, ...) \
  do { \
      if ((_NOX_DEBUG_ENABLED & (level))) { \
          const char *_lvl_str = \
              ((level) & DEBUG_VERIFY) ? "verify" : \
              ((level) & DEBUG_INFO)   ? "info"   : \
              ((level) & DEBUG_ERROR)  ? "error"  : \
              ((level) & DEBUG_MEMORY) ? "mem"    : "unknown"; \
          if ((function != NULL) && (function)[0]) \
              printf("[%s - %s (%s)] ", (module), (function), _lvl_str); \
          else if ((module) && (module)[0]) \
              printf("[%s (%s)] ", (module), _lvl_str); \
          printf(__VA_ARGS__); \
      } \
  } while (0)        

#define _NOX_DEBUG_TREE(branch, ...) \
  printf("[DEBUG - %s] " branch "\n", __func__, ##__VA_ARGS__);
// ============================
