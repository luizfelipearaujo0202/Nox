#pragma once
// parser/var/var_table.h

#include "var.h"
#include "../../structures/structures.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define _NOX_MAX_REHASH_ATTEMPTS 3

#define _NOX_HASHTABLE_HASH_ROTL64(x, b) \
    (((x) << (b)) | ((x) >> (64 - (b))))
#define _NOX_HASHTABLE_SIPROUND           \
  do {                                    \
    v0 += v1; v1 =                        \
      _NOX_HASHTABLE_HASH_ROTL64(v1, 13); \
    v1 ^= v0; v0 =                        \
      _NOX_HASHTABLE_HASH_ROTL64(v0, 32); \
    v2 += v3; v3 =                        \
      _NOX_HASHTABLE_HASH_ROTL64(v3, 16); \
    v3 ^= v2;                             \
    v0 += v3; v3 =                        \
      _NOX_HASHTABLE_HASH_ROTL64(v3, 21); \
    v3 ^= v0;                             \
    v2 += v1; v1 =                        \
      _NOX_HASHTABLE_HASH_ROTL64(v1, 17); \
    v1 ^= v2; v2 =                        \
      _NOX_HASHTABLE_HASH_ROTL64(v2, 32); \
  } while(0)


/* 
'buckets': array de ponteiros 
  para nodes da hash table

'capacity': capacidade do array

'count': quantidade de buckets
  ocupados
*/
  typedef 
struct {
  __NoxVar__** buckets;
  size_t capacity;
  size_t count;
  size_t _bucket_size;
  size_t max_bucket_size;
  uint8_t hash_key[16];
} __NoxVar_HashTable;



static inline __NoxValue_BOOLEAN_TYPE
  __NoxVar_HashTable_is_valid
    (const __NoxVar_HashTable* t) {
  if (t == NULL)  
    { return __NoxBool_FALSE_VALUE; }

  else if (
    t->buckets == NULL &&
    (t->capacity > 0 || t->count > 0)
  ) { return __NoxBool_FALSE_VALUE; }

  else if (t->buckets != NULL && 
    t->capacity == 0)
    { return __NoxBool_FALSE_VALUE; }

  else if (t->count > t->capacity)
    { return __NoxBool_FALSE_VALUE; }
  return __NoxBool_TRUE_VALUE;
}


// Siphash 2-4
static inline uint64_t
  __NoxVar_HashTable_hash(
    const __NoxVar_HashTable* t,
    const void* data,
    size_t len
  )
{
  const unsigned char* in = 
    (const unsigned char*)data;
  // k0 and k1
  uint64_t k0 = 0;
  uint64_t k1 = 0;
  for (int i = 0; i < 16; i++) {
    if (i < 8) {
      k0 |= ((uint64_t)t->hash_key[i]) << (8 * i);
    } else {
      k1 |= ((uint64_t)t->hash_key[i]) << (8 * (i - 8));
    }
  }

  // constant initializers
  uint64_t v0 = k0 ^ 0x736f6d6570737564ULL;
  uint64_t v1 = k1 ^ 0x646f72616e646f6dULL;
  uint64_t v2 = k0 ^ 0x6c7967656e657261ULL;
  uint64_t v3 = k1 ^ 0x7465646279746573ULL;

  uint64_t b = ((uint64_t)len) << 56;
  const size_t left = len & 7;
  const unsigned char* end = in + len - left;

  // 8 byte per block processing
  for (; in != end; in += 8) {
    uint64_t m;
    // loads 8 little-endian bytes in a safe way
    m = 
    (uint64_t)in[0] | ((uint64_t)in[1] << 8) |
    ((uint64_t)in[2] << 16) | ((uint64_t)in[3] << 24) |
    ((uint64_t)in[4] << 32) | ((uint64_t)in[5] << 40) |
    ((uint64_t)in[6] << 48) | ((uint64_t)in[7] << 56);

    v3 ^= m;
    _NOX_HASHTABLE_SIPROUND; // 2 rounds
    _NOX_HASHTABLE_SIPROUND;
    v0 ^= m;
  }

  // Last block processing
  uint64_t last = 0;
  switch (left) {
    case 7: last |= ((uint64_t)in[6]) << 48; // fallthrough
    case 6: last |= ((uint64_t)in[5]) << 40; // fallthrough
    case 5: last |= ((uint64_t)in[4]) << 32; // fallthrough
    case 4: last |= ((uint64_t)in[3]) << 24; // fallthrough
    case 3: last |= ((uint64_t)in[2]) << 16; // fallthrough
    case 2: last |= ((uint64_t)in[1]) << 8;  // fallthrough
    case 1: last |= ((uint64_t)in[0]);
  }

  b |= last;
  v3 ^= b;
  _NOX_HASHTABLE_SIPROUND; // 2 rounds
  _NOX_HASHTABLE_SIPROUND;
  v0 ^= b;

  // end (4 rounds)
  v2 ^= 0xff;
  _NOX_HASHTABLE_SIPROUND;
  _NOX_HASHTABLE_SIPROUND;
  _NOX_HASHTABLE_SIPROUND;
  _NOX_HASHTABLE_SIPROUND;

  return v0 ^ v1 ^ v2 ^ v3;
}


static inline size_t
  __NoxVar_HashTable_get_bucket
    ( __NoxVar_HashTable* t,
      const char* key) {
  if (t->capacity == 0)
    { return SIZE_MAX; }
  return \
    __NoxVar_HashTable_hash(t, key, strlen(key))
    % t->capacity;
}



static inline __NoxValue_BOOLEAN_TYPE
  __NoxVar_HashTable_clear
    (__NoxVar_HashTable* t)
  {
    if (!__NoxVar_HashTable_is_valid(t))
      { return __NoxBool_FALSE_VALUE; }

    for (size_t i = 0; i < t->capacity; i++) {
      __NoxVar__* current =
        t->buckets[i];

      while (current != NULL) {
        __NoxVar__* next = 
          current->next;
        free(current->name);

        __NoxVar_value_release(current->value);

        free(current);
        current = next;
      }
    }

    free(t->buckets);
    t->buckets = NULL;
    t->capacity = 0;
    t->count = 0;

    return __NoxBool_TRUE_VALUE;
  }

static inline __NoxValue_BOOLEAN_TYPE
  __NoxVar_HashTable_free
    (__NoxVar_HashTable* t)
  {
    if (!__NoxVar_HashTable_clear(t))
      { return __NoxBool_FALSE_VALUE; }
    free(t);
    return __NoxBool_TRUE_VALUE;
  }



static inline __NoxValue_BOOLEAN_TYPE
  __NoxVar_HashTable_realloc
    (__NoxVar_HashTable* t,
      const size_t capacity)
{
  if (!__NoxVar_HashTable_is_valid(t))
    { return __NoxBool_FALSE_VALUE; }

  size_t cap = capacity;

  if (cap == 0)
    { cap = (t->capacity == 0) ? 4 : t->capacity * 2; }

  if (cap < t->capacity ||
      cap > SIZE_MAX / sizeof(__NoxVar__*))
    { return __NoxBool_FALSE_VALUE; }

  if (cap == t->capacity)
    { return __NoxBool_TRUE_VALUE; }

  void* data = realloc(
    t->buckets,
    cap * sizeof(__NoxVar__*)
  );

  if (data == NULL)
    { return __NoxBool_FALSE_VALUE; }

  t->buckets = (__NoxVar__**)data;

  for (size_t i = t->capacity; i < cap; i++)
    { t->buckets[i] = NULL; }

  t->capacity = cap;

  return __NoxBool_TRUE_VALUE;
}



// Forward declaration for create's use
static inline __NoxValue_BOOLEAN_TYPE
  __NoxVar_HashTable_resize
    (__NoxVar_HashTable* t, size_t size);
// -----
static inline __NoxVar_HashTable*
  __NoxVar_HashTable_create(
    const size_t capacity,
    const size_t initial_bucket_size,
    const size_t max_bucket_size
  ) 
{
  if (max_bucket_size == 0 ||
      initial_bucket_size > max_bucket_size)
    { return NULL; }

  __NoxVar_HashTable* table = 
    (__NoxVar_HashTable*)malloc \
      (sizeof(__NoxVar_HashTable));

  if (table == NULL)
    { return NULL; }

  table->buckets = NULL;
  table->capacity = 0;
  table->count = 0;
  table->_bucket_size = 
    (initial_bucket_size == 0) ? 4 : initial_bucket_size;
  table->max_bucket_size = max_bucket_size;

  // Random bytes to initialize the 16 bytes key
  // of hash function
  FILE* f = fopen("/dev/urandom", "rb");
  if (f != NULL) {
    fread(table->hash_key, 1, 16, f);
    fclose(f);
  } else {
    // Fallback
    memset(table->hash_key, 0xAA, 16);
  }

  // Default table capacity: 4
  if (!__NoxVar_HashTable_realloc(table, capacity))
    { free(table);
      return NULL; }

  return table;
}


static inline __NoxValue_BOOLEAN_TYPE
  __NoxVar_HashTable_resize
    (__NoxVar_HashTable* t,
      size_t size)
{
  if (!__NoxVar_HashTable_is_valid(t) ||
      t->capacity == 0 ||
      size == 0 ||
      size < t->count)
    { return __NoxBool_FALSE_VALUE; }

  if (size == t->capacity)
    { return __NoxBool_TRUE_VALUE; }

  if (size > SIZE_MAX / sizeof(__NoxVar__*) ||
      t->capacity > SIZE_MAX / sizeof(__NoxVar__*))
    { return __NoxBool_FALSE_VALUE; }

  for (int attempt = 0; attempt < _NOX_MAX_REHASH_ATTEMPTS; attempt++) {
    __NoxVar_HashTable* new_t =
      __NoxVar_HashTable_create(
        size,
        t->_bucket_size,
        t->max_bucket_size
      );

    if (new_t == NULL)
      { return __NoxBool_FALSE_VALUE; }

    __NoxValue_BOOLEAN_TYPE failed =
      __NoxBool_FALSE_VALUE;

    for (size_t i = 0; i < t->capacity && !failed; i++) {

      __NoxVar__* old_current = t->buckets[i];

      while (old_current != NULL) {

        // Node allocation and data copy
        __NoxVar__* node =
          __NoxVar_create(
            old_current->name,
            old_current->value,
            old_current->explicit_type,
            old_current->declared_type,
            NULL
          );

        if (node == NULL) {
          failed = __NoxBool_TRUE_VALUE;
          break;
        }

        /*
        If bucket_count hits max_bucket_size,
          it tries again
        If all attempts done, returns false
        */

        size_t index =
          __NoxVar_HashTable_get_bucket(
            new_t,
            old_current->name
          );

        if (index == SIZE_MAX) {
          __NoxVar_free_single(node);
          failed = __NoxBool_TRUE_VALUE;
          break;
        }

        /*
        Principal logic.
        if new bucket is empty, it puts the node at first
          position
        else it adds the node at the last position
        */

        __NoxVar__* current = new_t->buckets[index];
        size_t bucket_count = 0;

        while (current != NULL) {
          bucket_count++;
          current = current->next;
        }

        if (bucket_count >= new_t->max_bucket_size) {
          __NoxVar_free_single(node);
          failed = __NoxBool_TRUE_VALUE;
          break;
        }

        if (new_t->buckets[index] == NULL) {
          new_t->buckets[index] = node;
          new_t->count++;
        }
        else {
          current = new_t->buckets[index];

          while (current->next != NULL)
            { current = current->next; }

          current->next = node;
        }

        old_current = old_current->next;
      }
    }

    if (failed) {
      __NoxVar_HashTable_clear(new_t);
      free(new_t);

      continue;
    }

    // Data transfer of 'new_t' to 't'
    __NoxVar_HashTable_clear(t);

    t->buckets = new_t->buckets;
    new_t->buckets = NULL;

    t->capacity = new_t->capacity;
    t->count = new_t->count;
    for (int i = 0; i < 16; i++) {
      t->hash_key[i] = new_t->hash_key[i];
    }

    free(new_t);

    return __NoxBool_TRUE_VALUE;
  }

  return __NoxBool_FALSE_VALUE;
}

static inline __NoxValue_BOOLEAN_TYPE
  __NoxVar_HashTable_insert
    (__NoxVar_HashTable* t, const __NoxVar__* var)
{
    if (!__NoxVar_HashTable_is_valid(t) ||
        t->capacity == 0 ||
        var == NULL || var->name == NULL ||
        var->value == NULL)
        return __NoxBool_FALSE_VALUE;

    __NoxVar__* found = NULL;
    __NoxVar__* current = NULL;
    __NoxVar__* last = NULL;
    size_t bucket_count = 0;
    size_t index = 0;

    // Search, rehash and search again if necessary
    for (int i = 0; i < _NOX_MAX_REHASH_ATTEMPTS; i++) {
        index = __NoxVar_HashTable_get_bucket(t, var->name);
        if (index == SIZE_MAX) return __NoxBool_FALSE_VALUE;
        current = t->buckets[index];

        found = NULL;
        last = NULL;
        bucket_count = 0;

        while (current != NULL) {
            if (strcmp(var->name, current->name) == 0) {
                found = current;
                break;
            }

            last = current;
            bucket_count++;
            current = current->next;
        }

        // Existing name: update its value
        if (found != NULL) {
            __NoxVar_value* v = __NoxVar_value_create(
                var->value->_t,
                var->value->_v
            );

            if (v == NULL)
                return __NoxBool_FALSE_VALUE;

            __NoxVar_value_release(found->value);
            found->value = v;

            return __NoxBool_TRUE_VALUE;
        }

        // Enough room: stop searching
        if (bucket_count < t->_bucket_size)
            break;

        // Avoid capacity overflow
        if (t->capacity > SIZE_MAX / 2)
            return __NoxBool_FALSE_VALUE;

        if (!__NoxVar_HashTable_resize(t, t->capacity * 2))
            return __NoxBool_FALSE_VALUE;

        // Last attempt was consumed by resize
        if (i == _NOX_MAX_REHASH_ATTEMPTS - 1)
            return __NoxBool_FALSE_VALUE;
    }

    // Check hard bucket limit
    if (bucket_count >= t->max_bucket_size)
        return __NoxBool_FALSE_VALUE;

    // Create new node
    __NoxVar__* node =
        __NoxVar_create(
            var->name,
            var->value,
            var->explicit_type,
            var->declared_type,
            NULL
        );

    if (node == NULL)
        return __NoxBool_FALSE_VALUE;

    // Insert into bucket
    if (last == NULL) {
        t->buckets[index] = node;
        t->count++;
    }
    else {
        last->next = node;
    }

    return __NoxBool_TRUE_VALUE;
}


static inline __NoxVar__*
  __NoxVar_HashTable_get(
    __NoxVar_HashTable* t,
    const char* name
  ) {
  if (!__NoxVar_HashTable_is_valid(t) ||
      name == NULL) 
    { return NULL; }

  size_t index = 
    __NoxVar_HashTable_get_bucket(t, name);
  if (index == SIZE_MAX) return NULL;

  __NoxVar__* current = t->buckets[index];

  while(current != NULL) {
    if (strcmp(current->name, name) == 0) 
      { return current; }
    current = current->next;
  }

  return NULL;
}



static inline __NoxValue_BOOLEAN_TYPE
  __NoxVar_HashTable_remove(
    __NoxVar_HashTable* t,
    const char* name
  ) {
    if (!__NoxVar_HashTable_is_valid(t) ||
        name == NULL) 
      { return __NoxBool_FALSE_VALUE; }
    if (t->capacity == 0)
      { return __NoxBool_FALSE_VALUE; }

    size_t index =
      __NoxVar_HashTable_get_bucket(t, name);
    if (index == SIZE_MAX) return __NoxBool_FALSE_VALUE;

    // Search for name
    __NoxVar__* last = NULL;
    __NoxVar__* current = t->buckets[index];
    while(current != NULL) { 
      if (strcmp(current->name, name) == 0)
        { break; }

      last = current;
      current = current->next; 
    }
    if (current == NULL)
      // name not found
      { return __NoxBool_FALSE_VALUE; }

    /*
    1. case:
                    ┌──────────┬──────────┬────────┐
    buckets[index]->│   LAST   |  FOUND   |  NULL  |
                    └──────────┴────┬─────┴────────┘
                                  CURRENT
    *->next appoints to the next node.
    If it's NULL, represents the end of
      the bucket.
    But if next is NULL and last is NULL
      too, it represents the start and 
      the end of it.
    */
    if (last == NULL) {
      /*
      First of the bucket
      Updates bucket to next node 
      */
      t->buckets[index] = current->next;
      if (current->next == NULL)
      { // decrease count because it's empty now
        t->count--; }
    }

    /*
    2. case:
                    ┌──────────┬──────────┬────────┐
    buckets[index]->│   LAST   |  FOUND   |  NODE  |
                    └──────────┴────┬─────┴────────┘
                                  CURRENT
    'current'->next it's a node.
    It needs to connects last->next
      to 'NODE' and release found
    If can't, returns false.
    */
    else {
      // Connecting 'last' to 'next'
      last->next = current->next;
    }

    // Now 'current' is free, it can be
    // released
    __NoxVar_free_single(current);

    return __NoxBool_TRUE_VALUE;
}