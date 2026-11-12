#pragma once
// token_array.h

#include "structures/structures.h"
#include <stdlib.h>

typedef struct {
    __NoxToken__* data;
    size_t count;
    size_t capacity;
} __NoxToken_array;


static inline __NoxValue_BOOLEAN_TYPE
  __NoxToken_array_push(
    __NoxToken_array* arr,
    __NoxToken__ token
) {
  if (arr->count == arr->capacity) {
    size_t cap = arr->capacity == 0
      ? 16
      : arr->capacity * 2;

    void* data = realloc(
      arr->data,
        cap * sizeof(__NoxToken__)
      );

    if (data == NULL) {
      return __NoxBool_FALSE_VALUE;
    }

    arr->data = (__NoxToken__*)data;
    arr->capacity = cap;
  }

  arr->data[arr->count++] = token;
  return __NoxBool_TRUE_VALUE;
}

static inline void __NoxToken_array_free(__NoxToken_array* arr) {
  free(arr->data);

  arr->data = NULL;
  arr->count = 0;
  arr->capacity = 0;
}