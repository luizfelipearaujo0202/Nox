#pragma once

// debug.h

#include "structures/ast_structures.h"
#include "structures/exec_structures.h"
#include "structures/structures.h"

static inline const char* show_token(const __NoxToken__* t)
  {
    if (t == NULL) return "NULL_TOKEN";
    switch (t->_type) {
        case NEW_LINE: return "NEW_LINE";

        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case STAR: return "STAR";
        case SLASH: return "SLASH";
        case PERCENT: return "PERCENT";

        case ASSIGN: return "ASSIGN";
        case PLUS_ASSIGN: return "PLUS_ASSIGN";
        case MINUS_ASSIGN: return "MINUS_ASSIGN";
        case STAR_ASSIGN: return "STAR_ASSIGN";
        case SLASH_ASSIGN: return "SLASH_ASSIGN";
        case PERCENT_ASSIGN: return "PERCENT_ASSIGN";

        case EQUAL: return "EQUAL";
        case NOT_EQUAL: return "NOT_EQUAL";
        case LESS: return "LESS";
        case LESS_EQUAL: return "LESS_EQUAL";
        case GREATER: return "GREATER";
        case GREATER_EQUAL: return "GREATER_EQUAL";

        case SHOW_LINE: return "SHOW_LINE";

        case LPAREN: return "LPAREN";
        case RPAREN: return "RPAREN";
        case LBRACE: return "LBRACE";
        case RBRACE: return "RBRACE";

        case LITERAL: return "LITERAL";
        case IDENTIFIER: return "IDENTIFIER";

        case IF: return "IF";
        case ELSE: return "ELSE";
        case WHILE: return "WHILE";
        case RETURN: return "RETURN";

        case AND: return "AND";
        case OR: return "OR";
        case NOT: return "NOT";

        case EOF_TOKEN: return "EOF_TOKEN";
        case INVALID_TOKEN: return "INVALID";
        default: return "UNKNOWN_TOKEN";
    }
  }

static inline const char* show_ast_type(const __NoxAST_NODE* n)
  {
    if (n == NULL) return "NULL_NODE";
    switch(n->_type) {
      case AST_LITERAL: return "AST_INT"; // Mantido conforme o seu print original
      case AST_IDENTIFIER: return "AST_IDENTIFIER";
      case AST_UNARY: return "AST_UNARY";
      case AST_BINARY: return "AST_BINARY";
      case AST_ASSIGNMENT: return "AST_ASSIGNMENT";
      default: return "UNKNOWN_AST_TYPE";
    }
  }

static inline const char* show_ast_operator(const __NoxAST_NODE* n) {
  if (n == NULL) return "NULL_NODE";
  __NoxAST_OPERATOR op = n->data.binary.op;
  switch (op) {
    case AST_DIVIDE: return "AST_DIVIDE";
    case AST_MINUS: return "AST_MINUS";
    case AST_MULTIPLY: return "AST_MULTIPLY";
    case AST_PLUS: return "AST_PLUS";
    default: return "UNKNOWN_OPERATOR";
  }
}

static inline const char* show_calc_status(const __NoxExec_CALC_STATUS s)
  {
    switch (s) {
      case CALC_OK: return "CALC_OK";
      case CALC_INVALID_ACCESS: return "CALC_INVALID_ACCESS";
      case CALC_INVALID_AST_TYPE: return "CALC_INVALID_AST_TYPE";
      case CALC_INVALID_EXPRESSION: return "CALC_INVALID_EXPRESSION";
      case CALC_OUT_OF_MEMORY: return "CALC_OUT_OF_MEMORY";
      case CALC_A_NOT_NUMBER: return "CALC_A_NOT_NUMBER";
      case CALC_B_NOT_NUMBER: return "CALC_B_NOT_NUMBER";
      case CALC_DIVISION_BY_ZERO: return "CALC_DIVISION_BY_ZERO";
      default: return "UNKNOWN_CALC_STATUS";
    }
  }