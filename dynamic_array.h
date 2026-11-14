#pragma once

#include "structures/structures.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>


static inline __NoxValue_BOOLEAN_TYPE
  __NoxDynaArr_is_valid
    (__NoxDynaArr__* arr)
    { if (arr == NULL || 
        arr->count > arr->capacity)
      { return __NoxBool_FALSE_VALUE; }

    if (arr->capacity == 0 && arr->data != NULL)
      { return __NoxBool_FALSE_VALUE; }

    else if (arr->element_size == 0)
      { return __NoxBool_FALSE_VALUE; }

    else if 
      ( arr->capacity > 0 &&
        arr->data == NULL ) 
      { return __NoxBool_FALSE_VALUE; }

    return __NoxBool_TRUE_VALUE; }


static inline __NoxValue_BOOLEAN_TYPE
  __NoxDynaArr_realloc
    (__NoxDynaArr__* arr) {
      if (!__NoxDynaArr_is_valid(arr))
      { return __NoxBool_FALSE_VALUE; }

    if (arr->count == arr->capacity) {
        size_t cap = 4;

        if (arr->capacity > SIZE_MAX / 2) 
          {return __NoxBool_FALSE_VALUE;}
        else cap = (arr->capacity == 0)
            ? 4
            : arr->capacity * 2;

        size_t size = 0;
        if (cap > \
          SIZE_MAX / arr->element_size) 
          { return __NoxBool_FALSE_VALUE; } 
        else size =
            cap * arr->element_size;

        void* data = realloc(
            arr->data,
            size
        );

        if (data == NULL) 
          { return __NoxBool_FALSE_VALUE; }

        arr->data = data;
        arr->capacity = cap;
    }

    return __NoxBool_TRUE_VALUE;
    }


static inline __NoxValue_BOOLEAN_TYPE
  __NoxDynaArr_clear (__NoxDynaArr__* arr)
    {
      if (!__NoxDynaArr_is_valid(arr))
        { return __NoxBool_FALSE_VALUE; }

      if (arr->destructor == NULL)
        { return __NoxBool_FALSE_VALUE; }

      for (size_t i=0; i < arr->count; i++) {
        arr->destructor(
          (char*)arr->data + i * arr->element_size
        );
      }
      free(arr->data);

      arr->count = 0;
      arr->capacity = 0;
      arr->data = NULL;
      return __NoxBool_TRUE_VALUE;
    }


static inline __NoxValue_BOOLEAN_TYPE
  __NoxDynaArr_free(__NoxDynaArr__* arr)
    { if (arr == NULL)
        { return __NoxBool_TRUE_VALUE; }
      
      if (!__NoxDynaArr_clear(arr))
        { return __NoxBool_FALSE_VALUE; }

      free(arr);
      return __NoxBool_TRUE_VALUE; }


static inline __NoxDynaArr__*
  __NoxDynaArr_create(
    size_t element_size,
    __NoxDynaArr_copy* copier,
    __NoxDynaArr_destructor* destructor,
    __NoxDynaArr_compare* comparator
  ) {
    if (element_size == 0)
      { return NULL; }

    __NoxDynaArr__* arr = 
      (__NoxDynaArr__*) \
      malloc(sizeof(__NoxDynaArr__));

    if (arr == NULL) 
      { return NULL; }

    arr->capacity = 0;
    arr->count = 0;
    arr->element_size = element_size;
    arr->copier = copier;
    arr->destructor = destructor;
    arr->comparator = comparator;
    arr->data = NULL;

    if (!__NoxDynaArr_is_valid(arr))
      { __NoxDynaArr_free(arr);
        return NULL; }

    return arr;
  }

static inline __NoxValue_BOOLEAN_TYPE
  __NoxDynaArr_push(
    __NoxDynaArr__* arr,
    const void* x,
    size_t size
  ) {
    if (x == NULL ||
        !__NoxDynaArr_is_valid(arr))
      { return __NoxBool_FALSE_VALUE; }

    else if (arr->element_size != size)
      { return __NoxBool_FALSE_VALUE; }

    else if (!__NoxDynaArr_realloc(arr))
      { return __NoxBool_FALSE_VALUE; }

    else if (arr->data == NULL)
      { return __NoxBool_FALSE_VALUE; }


    char* pos =
      (char*)arr->data + arr->count * arr->element_size;
      
    if (arr->copier != NULL) {
      if (!arr->copier(pos, x))
        { return __NoxBool_FALSE_VALUE; }
    } else {
      memcpy(pos, x, size);
    }

    arr->count++;
    return __NoxBool_TRUE_VALUE;
  }


static inline void*
  __NoxDynaArr_get(__NoxDynaArr__* arr, 
    size_t index)
  {
    if (!__NoxDynaArr_is_valid(arr))
      { return NULL; }

    else if (
      arr->data == NULL ||
      index >= arr->count
    )
      { return NULL; }

    return (char*) \
      arr->data + index * arr->element_size;
  }

static inline __NoxValue_BOOLEAN_TYPE
  __NoxDynaArr_remove(__NoxDynaArr__* arr,
    size_t index)
  {
    void* element = 
      __NoxDynaArr_get(arr, index);
    if (element == NULL) 
      { return __NoxBool_FALSE_VALUE; }

    if (arr->destructor == NULL)
      { return  __NoxBool_FALSE_VALUE; }
    arr->destructor(element);

    char* dest = (char*) \
      arr->data + index * arr->element_size;
    char* org = (char*) \
      dest + arr->element_size;
    memmove(dest, org,
      (arr->count - index - 1) * arr->element_size);

    arr->count--;

    return __NoxBool_TRUE_VALUE;
  }


static inline int
  __NoxDynaArr_find(__NoxDynaArr__* arr,
      const void* criteria) {
    if (!__NoxDynaArr_is_valid(arr) ||
        arr->comparator == NULL ||
        criteria == NULL)
      { return -1; }

    for (size_t i = 0; i < arr->count; i++)
    {
      void* object =
        (char*)arr->data + i * arr->element_size;
      if (arr->comparator(object, criteria))
        { return i; }
    }

    return -1;
  }

static inline int
  __NoxDynaArr_find_last(__NoxDynaArr__* arr,
    const void* criteria) {
    if (!__NoxDynaArr_is_valid(arr) ||
        arr->comparator == NULL ||
        criteria == NULL)
      { return -1; }

    for (size_t i = arr->count-1; i != 0; i--)
    {
      void* object =
        (char*)arr->data + i * arr->element_size;
      if (arr->comparator(object, criteria))
        { return i; }
    }

    return -1;
  }