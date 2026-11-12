#pragma once

#include "structures/structures.h"
#include "error/error_registry.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// lexer.h

typedef struct {
  const char* source;

  size_t line;
  size_t column;

  size_t pos;

  __NoxError_context* errors;
} __NoxLexer_pos;

static inline __NoxToken__ __NoxLexer_next(__NoxLexer_pos* p) {
  if (p == NULL || p->source == NULL || p->errors == NULL)
  { return (__NoxToken__){._type = INVALID_TOKEN}; }

  while (
  p->source[p->pos] == ' ' ||
  p->source[p->pos] == '\t'
  ) { p->column++; p->pos++; }

  char c = p->source[p->pos];

  __NoxToken__ t;
  t.start = &p->source[p->pos];
  t.length = 1;

  t.line = p->line;
  t.column = p->column;

  t._type = INVALID_TOKEN;
  t._literal_type = VALUE_NONE;

  if (c == '\n') {
    t._type = NEW_LINE;
    p->line++;
    p->column = 1;
    p->pos++;
    return t;
  }
  else if (c == '\r' && p->source[p->pos + 1] == '\n') {
    t._type = NEW_LINE;
    p->line++;
    p->column = 1;
    p->pos += 2;
    return t;
  }

  else if (c == '+') {
    p->pos++;
    p->column++;

    if (p->source[p->pos] == '=') {
      t._type = PLUS_ASSIGN;
      p->pos++;
      p->column++;
    }
    else {
      t._type = PLUS;
    }
  }
  else if (c == '-') {
    p->pos++;
    p->column++;

    if (p->source[p->pos] == '=') {
      t._type = MINUS_ASSIGN;
      p->pos++;
      p->column++;
    }
    else {
      t._type = MINUS;
    }
  }
  else if (c == '*') {
    p->pos++;
    p->column++;

    if (p->source[p->pos] == '=') {
      t._type = STAR_ASSIGN;
      p->pos++;
      p->column++;
    }
    else {
      t._type = STAR;
    }
  }
  else if (c == '/') {
    p->pos++;
    p->column++;

    if (p->source[p->pos] == '=') {
      t._type = SLASH_ASSIGN;
      p->pos++;
      p->column++;
    }
    else {
      t._type = SLASH;
    }
  }
  else if (c == '%') {
    p->pos++;
    p->column++;

    if (p->source[p->pos] == '=') {
      t._type = PERCENT_ASSIGN;
      p->pos++;
      p->column++;
    }
    else {
      t._type = PERCENT;
    }
  }


  else if (c == '=') {
    p->pos++;
    p->column++;

    if (p->source[p->pos] == '=') {
      t._type = EQUAL;
      p->pos++;
      p->column++;
    } else {
      t._type = ASSIGN;
    }
  }
  else if (c == '!') {
    p->pos++;
    p->column++;

    if (p->source[p->pos] == '=') {
      t._type = NOT_EQUAL;
      p->pos++;
      p->column++;
    } else {
      t._type = NOT;
    }
  }
  else if (c == '<') {
    p->pos++;
    p->column++;

    if (p->source[p->pos] == '=') {
      t._type = LESS_EQUAL;
      p->pos++;
      p->column++;
    } else {
      t._type = LESS;
    }
  }
  else if (c == '>') {
    p->pos++;
    p->column++;

    if (p->source[p->pos] == '=') {
      t._type = GREATER_EQUAL;
      p->pos++;
      p->column++;
    } else {
      t._type = GREATER;
    }
  }


  else if (c == '(') {
    t._type = LPAREN;
    p->pos++;
    p->column++;
  }
  else if (c == ')') {
    t._type = RPAREN;
    p->pos++;
    p->column++;
  }
  else if (c == '{') {
    t._type = LBRACE;
    p->pos++;
    p->column++;
  }
  else if (c == '}') {
    t._type = RBRACE;
    p->pos++;
    p->column++;
  }


  else if (isdigit((unsigned char)c)) {
    t._type = LITERAL;
    t._literal_type = VALUE_INT_LITERAL;

    while (isdigit((unsigned char)p->source[p->pos])) {
      p->pos++;
      p->column++;
    }
  }
  // STRING
  else if (c == '"') {
      t._type = LITERAL;
      t._literal_type = VALUE_STRING_LITERAL;
      p->pos++; // consume "
      while (
        p->source[p->pos] != '"' &&
        p->source[p->pos] != '\0'
      )
      { if (p->source[p->pos] == '\n')
          { break; }
        p->pos++; p->column++; }

      if (p->source[p->pos] == '\0') {
        t.length = &p->source[p->pos] - t.start;
        char msg[64];
        snprintf(msg, sizeof(msg),
          "unterminated string at line %zu",
          t.line
        );

        __NoxError__* e =
          __NoxError_create(
          ERROR_LEXER,
          msg,
          t.line,
          t.column,
          t.column + t.length
        );

        if (e == NULL)
          {
            _NOX_DEBUG_LOG(
              DEBUG_ERROR, "LEXER",
              "next", "criação do erro falhou\n"
            );
          }

        else if (!__NoxError_register(
            p->errors->registry,
            e,
            __NoxBool_TRUE_VALUE
          ))
          {
            _NOX_DEBUG_LOG(
              DEBUG_ERROR, "LEXER",
              "next", "criação do registro falhou\n"
            );
          }

        __NoxError_free(e);
        t._type = INVALID_TOKEN;
        t._literal_type = VALUE_NONE;
      }
      else if (p->source[p->pos] != '"') {
        t.length = &p->source[p->pos] - t.start;
        char msg[64];
        char str[2] = {c , '\0'};
        snprintf(msg, sizeof(msg),
          "unexpected character '%s' in string at %zu",
          (p->source[p->pos] == '\n' ? "\\n" : str),
          t.line
        );

        __NoxError__* e =
          __NoxError_create(
          ERROR_LEXER,
          msg,
          t.line,
          t.column,
          t.column + t.length
        );

        if (e == NULL)
          {
            _NOX_DEBUG_LOG(
              DEBUG_ERROR, "LEXER",
              "next", "criação do erro falhou\n"
            );
          }

        else if (!__NoxError_register(
            p->errors->registry,
            e,
            __NoxBool_TRUE_VALUE
          ))
          {
            _NOX_DEBUG_LOG(
              DEBUG_ERROR, "LEXER",
              "next", "criação do registro falhou\n"
            );
          }
        __NoxError_free(e);

        t._type = INVALID_TOKEN;
        t._literal_type = VALUE_NONE;
      }
      else p->pos++; // consume last "
      t.length = &p->source[p->pos] - t.start;
      return t;
  }
  else if (isalpha((unsigned char)c) || c == '_') {
    while (isalnum(p->source[p->pos]) || c == '_')
      { p->pos++; p->column++; }
    t.length = &p->source[p->pos] - t.start;

    if (
      t.length == 4 &&
      strncmp(t.start, "null", (int)t.length) == 0
    ) {
      t._type = LITERAL;
      t._literal_type = VALUE_NULL_LITERAL;
    }

    else if (
      t.length == 4 &&
      strncmp(t.start, "true", (int)t.length) == 0
    ) {
      t._type = LITERAL;
      t._literal_type = VALUE_TRUE;
    }

    else if (
      t.length == 5 &&
      strncmp(t.start, "false", (int)t.length) == 0
    ) {
      t._type = LITERAL;
      t._literal_type = VALUE_FALSE;
    }

    // KEYWORDS 
    else if (
      t.length == 2 &&
      strncmp(t.start, "if", (int)t.length) == 0
    ) {
      t._type = IF;
    }
    else if (
      t.length == 4 &&
      strncmp(t.start, "else", (int)t.length) == 0
    ) {
      t._type = ELSE;
    }
    else if (
      t.length == 5 &&
      strncmp(t.start, "while", (int)t.length) == 0
    ) {
      t._type = WHILE;
    }
    else if (
      t.length == 6 &&
      strncmp(t.start, "return", (int)t.length) == 0
    ) {
      t._type = RETURN;
    }

    else if (
      t.length == 3 &&
      strncmp(t.start, "and", (int)t.length) == 0
    ) {
      t._type = AND;
    }
    else if (
      t.length == 3 &&
      strncmp(t.start, "not", (int)t.length) == 0
    ) {
      t._type = NOT;
    }
    else if (
      t.length == 2 &&
      strncmp(t.start, "or", (int)t.length) == 0
    ) {
      t._type = OR;
    }
    else {
      t._type = IDENTIFIER;
    }

    return t;
  }


  else if (c == '\0') {
    t._type = EOF_TOKEN;
    return t;
  }
  else {

    char msg[64];
    snprintf(msg, sizeof(msg),
      "invalid token '%.*s' at line %zu",
      (int)t.length,
      t.start,
      t.line
    );

    __NoxError__* e =
      __NoxError_create(
      ERROR_LEXER,
      msg,
      t.line,
      t.column,
      t.column + t.length
    );

    if (e == NULL)
    {
      _NOX_DEBUG_LOG(
        DEBUG_ERROR, "LEXER",
        "next", "criação do erro falhou\n"
      );
    }

    else if (!__NoxError_register(
        p->errors->registry,
        e,
        __NoxBool_TRUE_VALUE
      ))
      {
        _NOX_DEBUG_LOG(
          DEBUG_ERROR, "LEXER",
          "next", "criação do registro falhou\n"
        );
      }

    p->pos++;
    p->column++;

    __NoxError_free(e);

    return t;
  }

  t.length = &p->source[p->pos] - t.start;

  return t;
}