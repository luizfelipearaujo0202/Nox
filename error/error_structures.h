#pragma once

#include "../structures/structures.h"
#include "../dynamic_array.h"
#include <stdlib.h>

  
  typedef 
struct {
  size_t start;
  size_t length;
} __NoxError_source_line;

  typedef 
struct {
  const char* src;
  __NoxError_source_line* line;
} __NoxError_source;

typedef enum {
ERROR_LEXER,
ERROR_SYNTAX,
ERROR_SEMANTIC,
ERROR_TYPE,
ERROR_NAME,
ERROR_RUNTIME
} __NoxError_TYPE;

  typedef 
struct {
  __NoxError_TYPE type;
  char* msg;
  int line;

  int column_start;
  int column_end;
} __NoxError__;

  typedef
struct {
  __NoxError__* error;
  size_t id;
  __NoxValue_BOOLEAN_TYPE fatal;
} __NoxError_registration;

  typedef 
struct {
  __NoxDynaArr__* data;
  size_t next_id;
} __NoxError_regarray;

typedef
  enum {
    NOX_REG_FIND_ID,
    NOX_REG_FIND_FATAL,
    NOX_REG_FIND_TYPE,
    NOX_REG_FIND_MSG
} __NoxError_regarray_find_mode;

typedef
  struct {
  __NoxError_regarray_find_mode mode;
  union {
    __NoxError_TYPE type;
    size_t id;
    __NoxValue_BOOLEAN_TYPE fatal;
    const char* msg;
  } expected;
} __NoxError_regarray_find_config;

  typedef
struct {
  __NoxError_regarray* registry;
} __NoxError_context;