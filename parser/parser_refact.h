#pragma once

// parser.h

#include "../structures/ast_structures.h"
#include "../structures/structures.h"
#include "../error/error_registry.h"
#include "../error.h"
#include "../debug.h"
#include "../mem_logger.h"
#include "var/var.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

/*
Auxiliar functions
*/

static inline __NoxError__*
__NoxParser_create_error(
    parser_info* p,
    __NoxError_TYPE type,
    int line,
    int c_start,
    int c_end,
    const char* format,
    ...
) {
  if (
    p == NULL ||
    p->errors == NULL ||
    p->errors->registry == NULL ||
    format == NULL
  )
    { return NULL; }

  char msg[512];
  va_list args;
  va_start(args, format);
  vsnprintf(msg, sizeof(msg), format, args);
  va_end(args);

  __NoxError__* e = __NoxError_create(
    type,
    msg,
    line,
    c_start,
    c_end,
    NULL
  );
  if (e == NULL)
    { return NULL; }

  if (!__NoxError_register(
    p->errors->registry,
    e,
    __NoxBool_TRUE_VALUE
  )) {
    __NoxError_free(e);
    return NULL;
  }
  return e;
}

static inline __NoxValue_BOOLEAN_TYPE
  __NoxParser_is_parserinfo_valid(parser_info* p) {
    if (p != NULL) {
      _NOX_DEBUG_LOG(
        DEBUG_VERIFY,
        "PARSER",
        "is_parserinfo_valid",
        "p=%p | tokens=%p | pos=%zu | count%zu\n",
        (void*)p, (void*)p->tokens, p->pos, p->count
      );
    }
    return (
      p == NULL ||
      p->tokens == NULL ||
      p->pos >= p->count ||
      p->count == 0
    )
    ? __NoxBool_FALSE_VALUE
    : __NoxBool_TRUE_VALUE;
  }

static inline __NoxValue_BOOLEAN_TYPE
  __NoxParser_is_token_null(__NoxToken__* t) {
    return (t == NULL || t->start == NULL)
    ? __NoxBool_TRUE_VALUE
    : __NoxBool_FALSE_VALUE;
  }

static inline  __NoxValue_BOOLEAN_TYPE
  __NoxParser_is_operator(
    __NoxToken__* t,
    int _is_plus,
    int _is_minus,
    int _is_star,
    int _is_slash,
    int _is_percent
  )
  { if (__NoxParser_is_token_null(t)) 
      { return __NoxBool_FALSE_VALUE; }
    return 
      ( (_is_plus == 1 && t->_type == PLUS) ||
        (_is_minus == 1 && t->_type == MINUS) ||
        (_is_star == 1 && t->_type == STAR) ||
        (_is_slash == 1 && t->_type == SLASH) ||
        (_is_percent == 1 && t->_type == PERCENT)
      ) 
      ? __NoxBool_TRUE_VALUE 
      : __NoxBool_FALSE_VALUE ; }

static inline void
  __NoxParser_ast_free(__NoxAST_NODE* n)
{
  if (n == NULL) return;

  switch (n->_type) {
    case AST_BINARY:
      __NoxParser_ast_free(n->data.binary.left);
      __NoxParser_ast_free(n->data.binary.right);
      break;

    case AST_ASSIGNMENT:
      __NoxParser_ast_free(n->data.assignment.identifier);
      __NoxParser_ast_free(n->data.assignment.value);
      break;

    case AST_IF:
      __NoxParser_ast_free(n->data.if_statement.condition);
      if (n->data.if_statement.then_branch)
        __NoxDynaArr_free(n->data.if_statement.then_branch);
      if (n->data.if_statement.else_branch)
        __NoxDynaArr_free(n->data.if_statement.else_branch);
      break;

    case AST_LITERAL:
    case AST_IDENTIFIER:
    case AST_UNARY:
    default:
      break;
  }

  _NOX_FREE(n);
}


static inline void
  __NoxParser_program_free(__NoxAST_PROGRAM* p)
{
  if (p == NULL) return;
  __NoxDynaArr_free(p->statements);
  _NOX_FREE(p);
}

// -----

/*
=======================
====     UTILS     ====
=======================
*/

/*
- Função: __NoxParser_peek

- Parâmetros:
  @p        - Informações do parser
  @n        - Deslocamento

- Objetivo:
Retornar um ponteiro para o token 
correspondente à posição definida 
por @n

- Funcionamento:
1. Verifica se @p é válido
  Se não: retorna NULL

2. Verifica se o deslocamento é
válido
  Se não: retorna NULL

3. Retorna o token definido por @n

- Retorno: 
  * Ponteiro para token (__NoxToken__*);
  ! NULL

- Notas:
  * @n pode ser negativo, representando
    posições antes da atual
*/
static inline __NoxToken__* 
  __NoxParser_peek
    (parser_info* p, ptrdiff_t n) {
  if (!__NoxParser_is_parserinfo_valid(p)) 
    { return NULL; }

  ptrdiff_t i = (ptrdiff_t)p->pos + n;
  if (i < 0 || i >= (ptrdiff_t)p->count)
    { return NULL; }

  return &p->tokens[i];
}

/*
- Função: __NoxParser_advance

- Parâmetros:
  @p        - Informações do parser
  @n        - Quantidade de avanço

- Objetivo:
Avançar a posição do parser em uma 
quantidade definida por @n e retornar
um ponteiro para o token correspondente.

- Funcionamento:
1. Verifica se @p é válido
  Se não: retorna NULL

2. Verifica se @n está dentro do limite
do array de tokens
  Se não: retorna NULL

3. Retorna o token correspondente ao offset
(@n)
  Se @n <= 1: retorna o próximo token
  Se não: soma @n á @p->pos e retorna o
    novo token atual

- Retorno: 
  * Ponteiro para token (__NoxToken__*);
  ! NULL

- Notas:
  * @n usa o tipo size_t (unsigned),
    portanto não representa valores negativos

  * @n == 0 e @n == 1 avançam apenas uma posição
*/
static inline __NoxToken__* 
  __NoxParser_advance
    (parser_info* p, size_t n) {
  if (!__NoxParser_is_parserinfo_valid(p))
    { return NULL; }

  else if (n > p->count)
    { return NULL; }

  else if (p->pos >= p->count - n) 
    { return NULL; }

  else { 
    if (n <= 1)
      {return &p->tokens[++p->pos];}
    else 
      { p->pos += n;
        return &p->tokens[p->pos];}
  }
}

/*
- Função: __NoxParser_match

- Parâmetros:
  @p        - Informações do parser
  @t        - Tipo do token esperado
  @offset   - Deslocamento baseado na posição 
              atual

- Objetivo:
Verificar se o tipo do token deslocado
(p->pos + @offset) corresponde ao tipo 
de @t

- Funcionamento:
1. Verifica se @p e @p->tokens existe
  Se não: retorna __NoxBool_FALSE_VALUE

2. Verifica se a posição resultante de
@p->pos + @offset está dentro do limite do
array de tokens
(< 0 ou >= @p->count)
  Se não: retorna __NoxBool_FALSE_VALUE

3. Captura o token localizado no deslocamento
@offset com __NoxParser_peek e guarda em 
$current
  Se o token for NULL: retorna
    __NoxBool_FALSE_VALUE

4. Compara o tipo de $current (*._type) com @t
  Se for igual: retorna __NoxBool_TRUE_VALUE
  Se não: retorna __NoxBool_FALSE_VALUE

- Retorno:
  Booleano (__NoxValue_BOOLEAN_TYPE)
  * __NoxBool_TRUE_VALUE
  ! __NoxBool_FALSE_VALUE

- Notas:
  * Não avança posição do parser
*/
static inline __NoxValue_BOOLEAN_TYPE
  __NoxParser_match(
    parser_info* p,
    __NoxToken_TYPE t,
    ptrdiff_t offset
) {
    if (p == NULL || p->tokens == NULL)
      { return __NoxBool_FALSE_VALUE; }

    if ((ptrdiff_t)p->pos + offset < 0 || 
        (ptrdiff_t)p->pos + offset >= 
          (ptrdiff_t)p->count)
      { return __NoxBool_FALSE_VALUE; }

    __NoxToken__* current = __NoxParser_peek(p, offset);
    if (current == NULL) 
      { return __NoxBool_FALSE_VALUE; }

    if (current->_type == t) 
      { return __NoxBool_TRUE_VALUE; } 

    return __NoxBool_FALSE_VALUE;
}


/*
- Função: __NoxParser_expect

- Parâmetros:
  @p        - Informações atuais do parser
  @expected - Tipo de token esperado
  @format   - Mensagem a ser formatada
  @...      - Argumentos variádicos utilizados
              na formatação da mensagem de erro

- Objetivo:
Verifica se o tipo do token atual corresponde
ao tipo de @expected.

- Funcionamento:
1. Verifica a correspondência utilizando
__NoxParser_match(p, expected, 0).
  Se houver correspondência:
    retorna __NoxBool_TRUE_VALUE.

3. Guarda o token atual em $token
utilizando __NoxParser_peek
  Se resultar em NULL: retorna
  __NoxBool_FALSE_VALUE

4. Cria um erro utilizando @format e os
    argumentos variádicos.

5. Registra o erro criado.
6. Retorna __NoxBool_FALSE_VALUE.

- Retorno:
  Booleano (__NoxValue_BOOLEAN_TYPE)
  * __NoxBool_TRUE_VALUE
  ! __NoxBool_FALSE_VALUE

- Notas:
  * Não avança a posição do parser.
  * Depende de um registro válido
    (p->errors->registry).
  * A mensagem formatada possui limite
    de 512 bytes.
*/
static inline __NoxValue_BOOLEAN_TYPE
  __NoxParser_expect(
    parser_info* p,
    __NoxToken_TYPE expected,
    const char* format,
    ...
  ) {
    if (p == NULL) return __NoxBool_FALSE_VALUE;

    if (__NoxParser_match(p, expected, 0))
      { return __NoxBool_TRUE_VALUE; }

    if (
      p->errors == NULL ||
      p->errors->registry == NULL
    ) {
      _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "PARSER",
        "expect",
        "p->errors ou p->errors->registry é nulo"
      );
      return __NoxBool_FALSE_VALUE;
    }

    char msg[512];

    va_list args;
    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);

    __NoxToken__* token =
      __NoxParser_peek(p, 0);

    if (token == NULL)
      { return __NoxBool_FALSE_VALUE; }

    __NoxError_free(
      __NoxParser_create_error(
        p,
        ERROR_SYNTAX,
        token->line,
        token->column,
        token->column +
          token->length,
        msg
      )
    );

    return __NoxBool_FALSE_VALUE;
  }
// =======================


/*
=======================
====    PARSING    ====
=======================
*/

/*
- Função: __NoxParse_primary

- Parâmetros:
  @p        - Informações do parser

- Objetivo: 
Analisar e construir o nó AST para elementos
primários (literais, identificadores ou
expressões entre parênteses)

- Funcionamento:
1. Verifica se @p é válido
  Se não: retorna NULL

2. Captura o token atual com __NoxParser_peek 
e guarda em $current
  Se NULL: retorna NULL

3. Aloca dinamicamente o nó AST em $node

4. Configura o nó de acordo com $current->_type
  - LITERAL: 
    Define o nó como AST_LITERAL
  - IDENTIFIER: 
    Define o nó como AST_IDENTIFIER
  - LPAREN ('('): 
    Consome o parêntese,
    avalia a expressão interna chamando
    __NoxParser_parse_expression e espera o
    fechamento com RPAREN (')')
  - Outros:
    Registra erro de sintaxe
    por token inesperado e retorna NULL

5. Avança o parser e retorna o node construído

- Retorno: 
  * Ponteiro para o nó AST (__NoxAST_NODE*)
  ! NULL

- Consumo de tokens:
  - LITERAL
  - IDENTIFIER
  - LPAREN
  - RPAREN

- Notas:
  * Responsável por consumir os tokens de menor
    hierarquia sintática (folhas da árvore).
*/
// Forward declaration for parse_expression(p)
static inline __NoxAST_NODE*
  __NoxParser_parse_expression(parser_info* p);

static inline __NoxAST_NODE*
  __NoxParser_parse_primary(parser_info* p) {
    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "PARSER",
      "parse_primary",
      "STARTED!\n"
    );
    
    if (!__NoxParser_is_parserinfo_valid(p))
      { _NOX_DEBUG_LOG(
          DEBUG_ERROR,
          "PARSER",
          "parse_primary",
          "parser_info (p) inválido\n"
        );
        return NULL; }

    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "PARSER",
      "parse_primary",
      "p: %s | p->tokens: %s\n",
      p ? "OK" : "NULL", p && p->tokens ? "OK" : "NULL"
    );

    __NoxValue_BOOLEAN_TYPE error_ok =
      __NoxBool_TRUE_VALUE;

    if (p->errors == NULL || 
        p->errors->registry == NULL)
      { error_ok = __NoxBool_FALSE_VALUE; }

    __NoxToken__* current = __NoxParser_peek(p, 0);

    _NOX_DEBUG_LOG(
      DEBUG_VERIFY,
      "PARSER",
      "parse_primary",
      "current: %s\n",
      current ? "OK" : "NULL"
    );
    if (current == NULL) 
      { return NULL; }

    __NoxAST_NODE* node = (__NoxAST_NODE*)\
        _NOX_ALLOC(sizeof(__NoxAST_NODE));

    if (node == NULL) 
      { _NOX_DEBUG_LOG(
          DEBUG_ERROR,
          "PARSER",
          "parse_primary",
          "erro ao alocar 'node'\n"
        );
        return NULL; }

    _NOX_DEBUG_LOG(
      DEBUG_VERIFY,
      "PARSER",
      "parse_primary",
      "token atual: %d | pos: %zu\n",
      current->_type, p->pos
    );

    if (current->_type == LITERAL) {
      // literal value
      node->_type = AST_LITERAL;
    }
    else if (current->_type == IDENTIFIER) {
      // variable
      node->_type = AST_IDENTIFIER;
    } 
    else if (current->_type == LPAREN) {
      // '('
      current = __NoxParser_advance(p, 0);
      _NOX_FREE(node);
      node = __NoxParser_parse_expression(p);
      _NOX_DEBUG_LOG(
        DEBUG_INFO,
        "PARSER",
        "parse_primary",
        "expressão retornou: %s | pos: %zu\n",
        node ? "OK" : "NULL", p->pos
      );

      if (node == NULL)
        { _NOX_DEBUG_LOG(
            DEBUG_ERROR,
            "PARSER",
            "parse_primary",
            "node = NULL\n"
          );
          return NULL; }

      current = __NoxParser_peek(p, 0);
      if (current == NULL) 
      { _NOX_DEBUG_LOG(
            DEBUG_ERROR,
            "PARSER",
            "parse_primary",
            "current = NULL\n"
          );
        __NoxParser_ast_free(node);
        return NULL; }

      // Token esperado: ')'
      else if (!__NoxParser_expect(
          p,
          RPAREN,
          "'(' not closed"
        )) {
        _NOX_DEBUG_LOG(
          DEBUG_INFO,
          "PARSER",
          "parse_primary",
          "current entrou no if de parênteses\n"
        );
        __NoxParser_ast_free(node);
        return NULL;
      }

      __NoxParser_advance(p, 0);
      return node;
    } 
    else { 
    if (error_ok) {
      if (!__NoxParser_expect(
        p, 
        EOF_TOKEN,
        "expected expression after '%.*s'",
        __NoxParser_peek(p, -1)->length,
        __NoxParser_peek(p, -1)->start
      )) { return NULL; }

      __NoxError__* e =
        __NoxParser_create_error(
          p, ERROR_SYNTAX,
          (int)current->line,
          (int)current->column,
          (int)(current->column),
          "unexpected token '%.*s'",
          (int)current->length, current->start
        );

      if (e == NULL) { return NULL;}

      _NOX_DEBUG_LOG(
        DEBUG_VERIFY,
        "PARSER",
        "parse_primary",
        "Registro: %p\n",
        (void*)e
      );
      _NOX_DEBUG_LOG(
        DEBUG_VERIFY,
        "PARSER",
        "parse_primary",
        "Primeiro fatal: %p\n",
        (void*)__NoxError_get_first(p->errors->registry)
      );

      __NoxError_free(e);
      _NOX_FREE(node); return NULL; 
    }
    else return NULL;
  }

  __NoxParser_advance(p, 0);
  node->data.token = current;
  return node;
} 



/*
--------------------------------
-- AUXILIAR PARSING FUNCTIONS --
--------------------------------
*/

static inline __NoxAST_NODE*
  __NoxParser_get_left_node(parser_info* p)
  { return __NoxParser_parse_primary(p); }

static inline __NoxAST_NODE*
  __NoxParser_get_right_node(parser_info* p)
  {
    if (!__NoxParser_is_parserinfo_valid(p))
      { return NULL; }
    if (p->pos > 0 && 
        p->count > 0 &&
        __NoxParser_is_operator(&p->tokens[p->pos-1], 1, 1, 1, 1, 1))
      { _NOX_DEBUG_LOG(
          DEBUG_INFO,
          "PARSER",
          "parse_primary",
          "OPERATOR; token type=%s | pos=%zu\n",
          __NoxDebug_show_token(__NoxParser_peek(p, -1)),
          p->pos
        );
        return __NoxParser_parse_primary(p); }
    return NULL;
  }

static inline __NoxValue_BOOLEAN_TYPE
  __NoxParser_get_ast_operator
        ( parser_info* p, 
          __NoxAST_OPERATOR* op) {
    if (!__NoxParser_is_parserinfo_valid(p) || op == NULL) 
      { return __NoxBool_FALSE_VALUE; }

    else if (!__NoxParser_is_operator(&p->tokens[p->pos], 1, 1, 1, 1, 1))
      { return __NoxBool_FALSE_VALUE; }

    __NoxToken_TYPE t = p->tokens[p->pos]._type;

    if (t == PLUS) *op = AST_PLUS;
    else if (t == MINUS) *op = AST_MINUS;
    else if (t == STAR) *op = AST_MULTIPLY;
    else *op = AST_DIVIDE;

    return __NoxBool_TRUE_VALUE;
  }

// --------------------------------

/*
- Função: __NoxParser_parse_factor

- Parâmetros:
  @p - Informações do parser

- Objetivo:
Analisar e construir nós AST para operações
de multiplicação e divisão, respeitando sua
precedência sobre adição e subtração.

- Funcionamento:
1. Valida @p.
  Se inválido: retorna NULL.

2. Obtém o nó esquerdo usando
__NoxParser_get_left_node.
  Se NULL: retorna NULL.

3. Captura o token atual com __NoxParser_peek
e guarda em $current.
  Se NULL: retorna $left.

4. Entra em loop enquanto $current representar
um operador de multiplicação ou divisão.

5. Para cada operador encontrado:
  - Determina o operador AST correspondente.
  - Avança o parser para consumir o operador.
  - Obtém o operando direito usando
    __NoxParser_get_right_node.
  - Se o operando direito for NULL:
    libera $left e retorna NULL.
  - Aloca um novo nó AST_BINARY.
  - Associa $left, o operador e o operando direito.
  - Atualiza $left para o novo nó.
  - Atualiza $current com o próximo token.

6. Retorna $left, que representa a raiz
da subárvore construída.

- Consumo de tokens:
  - STAR
  - DIVIDE
  - Tokens consumidos por
    __NoxParser_get_left_node e
    __NoxParser_get_right_node.

- Retorno:
  * Ponteiro para o nó AST (__NoxAST_NODE*)
  ! NULL

- Notas:
  * Implementa associatividade à esquerda.
  * Respeita a precedência de multiplicação
    e divisão sobre adição e subtração.
  * Libera os nós já construídos em caso
    de falha ao obter o operando direito
    ou alocar o novo nó.
*/
static inline __NoxAST_NODE*
  __NoxParser_parse_factor(parser_info* p) {
    if (!__NoxParser_is_parserinfo_valid(p))
      { return NULL; }

    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "PARSER",
      "parse_factor",
      "pos=%zu | count=%zu\n",
      p->pos, p->count
    );

    __NoxAST_NODE* left = __NoxParser_get_left_node(p);
    if (left == NULL) 
      { _NOX_DEBUG_LOG(
          DEBUG_ERROR,
          "PARSER",
          "parse_factor",
          "'left' is null\n"
        );
        return NULL; }

    __NoxToken__* current = __NoxParser_peek(p, 0);
    if (current == NULL)
      { return left; }

    while(__NoxParser_is_operator(current, 0, 0, 1, 1, 1)) {
      __NoxAST_OPERATOR op;
      if (__NoxParser_is_operator(current, 0, 0, 1, 0, 0))
      { op = AST_MULTIPLY; }
      else if (
        __NoxParser_is_operator(current, 0, 0, 0, 1, 0)
      )
      { op = AST_DIVIDE; }
      else {op = AST_MODULE;}

      __NoxParser_advance(p, 0);

      __NoxAST_NODE* right = __NoxParser_get_right_node(p);
      if (right == NULL)
        { __NoxParser_ast_free(left);
          _NOX_DEBUG_LOG(
            DEBUG_ERROR,
            "PARSER",
            "parse_factor",
            "'right' is null\n"
          );
          return NULL; }

      __NoxAST_NODE* node = 
        (__NoxAST_NODE*)_NOX_ALLOC(sizeof(__NoxAST_NODE));
      if (node == NULL) 
        { __NoxParser_ast_free(left);
          __NoxParser_ast_free(right);
          return NULL; }

      node->_type = AST_BINARY;
      node->data.binary.left = left;
      node->data.binary.op = op;
      node->data.binary.right = right;

      left = node;
      current = __NoxParser_peek(p, 0);
    }

    return left;
  }


/*
- Função: __NoxParser_parse_expression

- Parâmetros:
  @p        - Informações do parser

- Objetivo:
Analisar e construir os nós AST para
operações aritméticas de adição e subtração

- Funcionamento:
1. Valida @p
  Se inválido: retorna NULL

2. Obtém $left usando __NoxParser_parse_factor
  Se NULL: retorna NULL

3. Coleta token atual com __NoxParser_peek e
guarda em $current
  se NULL: retorna $left

4. Entra em loop enquanto o token atual for um
operador aditivo (+ ou -)

5. Para cada operador:
  - Identifica operador AST via 
    __NoxParser_get_ast_operator; se falso, 
    retorna NULL
  - Avança o parser
  - Obtém $right usando __NoxParser_parse_factor;
    se NULL: libera $left e retorna NULL
  - Combina ambos em um novo nó AST do tipo 
    AST_BINARY

6. Retorna a árvore resultante $left

- Consumo de tokens:
  - PLUS
  - MINUS
  - Tokens de __NoxParser_parse_factor

- Retorno:
  * Ponteiro para o nó AST (__NoxAST_NODE*)
  ! NULL

- Notas:
  * Implementa associatividade à esquerda.
  * Possui precendência menor que os fatores
    (multiplicação e divisão)
*/
static inline __NoxAST_NODE*
  __NoxParser_parse_expression(parser_info* p) {
    if (!__NoxParser_is_parserinfo_valid(p))
      { return NULL; }

    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "PARSER",
      "parse_expression",
      "pos=%zu | count=%zu\n",
      p->pos, p->count
    );

    __NoxAST_NODE* left =__NoxParser_parse_factor(p);
    if (left == NULL) 
      { _NOX_DEBUG_LOG(
          DEBUG_ERROR,
          "PARSER",
          "parse_expression",
          "'left' is null\n"
        );
        return NULL; }

    __NoxToken__* current = __NoxParser_peek(p, 0);
    if (current == NULL)
      { return left; }

    while(__NoxParser_is_operator(current, 1, 1, 0, 0, 0)) {
      __NoxAST_OPERATOR op;
      if (!__NoxParser_get_ast_operator(p, &op)) 
        { __NoxParser_ast_free(left);
          return NULL; }

      __NoxParser_advance(p, 0);

      __NoxAST_NODE* right = __NoxParser_parse_factor(p);
      if (right == NULL)
        { __NoxParser_ast_free(left);
          _NOX_DEBUG_LOG(
            DEBUG_ERROR,
            "PARSER",
            "parse_expression",
            "'right' is null\n"
          );
          return NULL; }

      __NoxAST_NODE* node = 
        (__NoxAST_NODE*)_NOX_ALLOC(sizeof(__NoxAST_NODE));
      if (node == NULL) 
        { __NoxParser_ast_free(left);
          __NoxParser_ast_free(right);
          return NULL; }

      node->_type = AST_BINARY;
      node->data.binary.left = left;
      node->data.binary.op = op;
      node->data.binary.right = right;

      left = node;
      current = __NoxParser_peek(p, 0);
    }
    return left;
  }

/*
- Função: __NoxParser_parse_statement

- Parâmetros:
  @p        - Informações do parser

- Objetivo:
Analisar e construir instruções completas
da linguagem, como atribuições

- Funcionamento:
1. Valida @p
  Se inválido: retorna NULL

2. Verifica se token atual não é um identificador mas
é um sinal de atribuição
  Se sim: registra erro de atribuição inesperada 
    e retorna NULL

3. Consome identificador com __NoxParser_parse_primary
  Se NULL: retorna NULL

4. Verifica se token atual é um sinal de atribuição
  Se não: libera $identifier e retorna NULL

5. Consome o sinal de atribuição

6. Consome valor com __NoxParser_parse_expression
  Se NULL: libera $identifier e retorna NULL

7. Verifica se token final é '\n' ou EOF
  Se não: registra erro de token inesperado após
    expressão, libera $identifier e $value e
    retorna NULL

8. Cria novo nó AST cujo tipo é AST_ASSIGNMENT
e atribui $identifier e $value ao nó

9. Retorna nó AST

- Consumo de tokens:
  - IDENTIFIER
  - ASSIGN
  - Tokens de __NoxParser_parse_primary
  - Tokens de __NoxParser_parse_expression

- Retorno:
  * Ponteiro para o nó AST (__NoxAST_NODE*)
  ! NULL
*/
static inline __NoxAST_NODE*
  __NoxParser_parse_statement(parser_info* p) 
{ if (!__NoxParser_is_parserinfo_valid(p))
    { return NULL; }

  if (!__NoxParser_match(p, IDENTIFIER, 0)) {
    if (__NoxParser_match(p, ASSIGN, 0)) {
      _NOX_DEBUG_LOG(
        DEBUG_VERIFY,
        "PARSER",
        "parse_statement",
        "erro de '=' inesperado"
      );

      __NoxParser_expect(
        p,
        IDENTIFIER,
        "unexpected '='; expected identifier"
      );
    }
    return NULL;
  }

  _NOX_DEBUG_LOG(
    DEBUG_VERIFY,
    "PARSER",
    "parse_statement",
    "consuming identifier\n"
  );
  __NoxAST_NODE* identifier =
    __NoxParser_parse_primary(p);

  if (identifier == NULL) 
    { _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "PARSER",
        "parse_statement",
        "error while consuming identifier\n"
      );
      return NULL; }

  /*
  Verifica se o tipo é explícito
  Se sim: guarda
  Se não: continua
  */
  __NoxVar_value_type decl_type = NONE_VALUE;
  __NoxValue_BOOLEAN_TYPE explicit_t = 
      __NoxBool_FALSE_VALUE;
  if (__NoxParser_match(p, PIPE, 0)) {
    explicit_t = __NoxBool_TRUE_VALUE;
    const __NoxToken__* t =
      __NoxParser_advance(p, 1);

    if (!__NoxParser_expect(
      p, 
      IDENTIFIER, 
      "invalid type '%.*s'",
      t->length, t->start))
    { __NoxParser_ast_free(identifier); 
      return NULL; }
    char* key = (char*)_NOX_ALLOC(t->length + 1);
    if (key == NULL)
      { __NoxParser_ast_free(identifier); return NULL; }

    memcpy(key, t->start, t->length);;
    key[t->length] = '\0';
    if (
      t->length == 3 &&
      strcmp(key, "int") == 0
    ) {
      decl_type = INT_VALUE;
    }
    else if (
      t->length == 5 &&
      strcmp(key, "float") == 0
    ) {
      decl_type = FLOAT_VALUE;
    }
    else if (
      t->length == 6 &&
      strcmp(key, "string") == 0
    ) {
      decl_type = STRING_VALUE;
    }
    else if (
      t->length == 4 &&
      strcmp(key, "null") == 0
    ) {
      decl_type = NULL_VALUE;
    }
    else if (
      t->length == 4 && 
      strcmp(key, "bool") == 0
    ) {
      decl_type = BOOL_VALUE;
    }
    else {
      __NoxError__* e =
        __NoxParser_create_error(
          p,
          ERROR_TYPE,
          t->line,
          t->column,
          t->column + t->length,
          "unknown type '%s'",
          key
        );

      __NoxError_free(e);
      __NoxParser_ast_free(identifier);
      _NOX_FREE(key);
      return NULL;
    }

    _NOX_FREE(key);
    __NoxParser_advance(p, 1);

    if (!__NoxParser_expect(
      p,
      PIPE,
      "expected '|'"
    )) { __NoxParser_ast_free(identifier); return NULL; }
    else {
      __NoxParser_advance(p, 1);
    }
  }

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "PARSER",
    "parser_statement",
    "identifier consumed\n"
  );


  _NOX_DEBUG_LOG(
    DEBUG_VERIFY,
    "PARSER",
    "parse_statement",
    "consuming assign\n"
  );
  if (!__NoxParser_match(p,ASSIGN, 0)) {
    _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "PARSER",
        "parse_statement",
        "error while consuming '='\n"
      );
    __NoxParser_ast_free(identifier);
    return NULL;
  }
  __NoxParser_advance(p, 1);
  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "PARSER",
    "parse_statement",
    "assign ('=') consumed\n"
  );

  _NOX_DEBUG_LOG(
    DEBUG_VERIFY,
    "PARSER",
    "parse_statement",
    "consuming value\n"
  );
  __NoxAST_NODE* value =
    __NoxParser_parse_expression(p);
  if (value == NULL) {
    _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "PARSER",
        "parse_statement",
        "error while consuming value\n"
      );
    __NoxParser_ast_free(identifier);
    return NULL;
  }

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "PARSER",
    "parser_statement",
    "value consumed\n"
  );

  if (!__NoxParser_match(p, NEW_LINE, 0) &&
      !__NoxParser_match(p, EOF_TOKEN, 0)) {

    _NOX_DEBUG_LOG(
      DEBUG_ERROR,
      "PARSER",
      "parse_statement",
      "unexpected token %s",
      (__NoxParser_match(p,NEW_LINE,0) ? "'\\n'" : "EOF")
    );

    if (p->errors != NULL && p->errors->registry != NULL)
    {
      __NoxToken__* t =
        __NoxParser_advance(p, 0);

      char msg[64];
      snprintf(msg, sizeof(msg),
        "unexpected token '%.*s' after assignment",
        (int)t->length, t->start
      );

      __NoxParser_expect(
        p,
        NEW_LINE,
        msg
      );
      __NoxParser_expect(
        p,
        EOF_TOKEN,
        msg
      );
    }

    __NoxParser_ast_free(identifier);
    __NoxParser_ast_free(value);
    return NULL;
  }

  __NoxAST_NODE* node = (__NoxAST_NODE*) \
    _NOX_ALLOC(sizeof(__NoxAST_NODE));
  if (node == NULL) {
    __NoxParser_ast_free(identifier);
    __NoxParser_ast_free(value);
    return NULL; 
  }

  node->_type = AST_ASSIGNMENT;
  node->data.assignment.identifier = identifier;
  node->data.assignment.value = value;

  node->data.assignment.explicit_type = explicit_t;
  node->data.assignment.type = decl_type;
  return node;
}


/*
=========================
===    CONDITIONAL    ===
=========================
- Contém funções que envolvem tokens condicionais:
  IF,
  ELSE,
  EQUAL,
  NOT_EQUAL,
  GREATER,
  LESS,
  GREATER_EQUAL,
  LESS_EQUAL
*/
/*
- Função: __NoxParser_parse_comparison

- Parâmetros:
  @p        - Informações do parser

- Objetivo:
Analisar uma condição e retornar um
nó relativo a ela

- Funcionamento:
1. Valida @p
  Se inválido: retorna NULL

2. Obtém $left com __NoxParser_parse_expression
  Se NULL: retorna NULL

3. Coleta $current com __NoxParser_peek
  Se NULL: retorna $left

4. Inicia loop principal
  - Identifica operador lógico
  - Consome-o
  - Obtém $right com __NoxParser_parse_expression
    Se NULL: retorna NULL
  - Aloca nó AST $node
    Se NULL: retorna NULL
  - Configura $node
  - Atribui $node a $left
  - Atribui o token atual a $current

5. Retorna $left

- Consumo de tokens:
  - EQUAL
  - NOT_EQUAL
  - GREATER
  - LESS
  - GREATER_EQUAL
  - LESS_EQUAL
  - Tokens de __NoxParser_parse_expression

- Retorno:
  * Ponteiro para o nó AST (__NoxAST_NODE*)
  ! NULL

- Notas:
  * Implementa associatividade à esquerda.
  * Possui precendência menor que operadores aritméticos
*/
static inline __NoxAST_NODE*                                                                                           
  __NoxParser_parse_comparison(parser_info* p)                                                                           
  {                      
    if (!__NoxParser_is_parserinfo_valid(p))
      { return NULL; }

    __NoxAST_NODE* left = __NoxParser_parse_expression(p);                                                             
    if (left == NULL) return NULL;

    __NoxToken__* current = __NoxParser_peek(p, 0);                                                                    
    if (current == NULL) return left;                                                                                  

    while (                                                                                                            
      current->_type == EQUAL ||                                                                                     
      current->_type == NOT_EQUAL ||
      current->_type == LESS ||
      current->_type == GREATER ||
      current->_type == LESS_EQUAL ||
      current->_type == GREATER_EQUAL
    ) {
      __NoxAST_OPERATOR op = AST_EQUAL; /* default seguro */
      switch (current->_type) {                                                                                      
        case EQUAL:         op = AST_EQUAL; break;
        case NOT_EQUAL:     op = AST_NOT_EQUAL; break;
        case LESS:          op = AST_LESS; break;                                                                  
        case GREATER:       op = AST_GREATER; break;                                                               
        case LESS_EQUAL:    op = AST_LESS_EQUAL; break;                                                            
        case GREATER_EQUAL: op = AST_GREATER_EQUAL; break;                                                         
          default: break;                                                                                            
      }                                                                                                              

      __NoxParser_advance(p, 0);

      __NoxAST_NODE* right = __NoxParser_parse_expression(p);
      if (right == NULL) {
        __NoxParser_ast_free(left);
        __NoxParser_ast_free(right);
        return NULL;
      }

      __NoxAST_NODE* node = (__NoxAST_NODE*)
        _NOX_ALLOC(sizeof(__NoxAST_NODE));
      if (node == NULL) {
        __NoxParser_ast_free(left);
        __NoxParser_ast_free(right);
        return NULL;
      }
      node->_type = AST_BINARY;
      node->data.binary.left = left;
      node->data.binary.op = op;
      node->data.binary.right = right;

      left = node;
      current = __NoxParser_peek(p, 0);
    }

    return left;
  }


static inline __NoxAST_NODE*
  __NoxParser_parse_logical_and
    (parser_info* p)
{
  if (!__NoxParser_is_parserinfo_valid(p))
    { return NULL;}

  __NoxAST_NODE* left = 
    __NoxParser_parse_comparison(p);
  if (left == NULL)
    { return NULL; }

  __NoxAST_NODE* node = NULL;
  while (__NoxParser_peek(p, 0)->_type == AND) {
    __NoxParser_advance(p, 0); // consome 'and'

    __NoxAST_NODE* right = __NoxParser_parse_comparison(p);
    if (right == NULL)
      { __NoxParser_ast_free(left);
        return NULL; }

    __NoxAST_OPERATOR op = AST_AND;

    node = (__NoxAST_NODE*)
      _NOX_ALLOC(sizeof(__NoxAST_NODE));
    if (node == NULL) {
      __NoxParser_ast_free(left);
      __NoxParser_ast_free(right);
      return NULL;
    }

    node->_type = AST_BINARY;
    node->data.binary.left = left;
    node->data.binary.op = op;
    node->data.binary.right = right;

    left = node;
  }
  return left;
}

static inline __NoxAST_NODE*
  __NoxParser_parse_logical_or
    (parser_info* p)
{
  if (!__NoxParser_is_parserinfo_valid(p))
    { return NULL; }

  __NoxAST_NODE* left = 
    __NoxParser_parse_logical_and(p);
  if (left == NULL)
    { return NULL; }

  while (__NoxParser_peek(p, 0)->_type == OR) {
    __NoxParser_advance(p, 0); // consome 'or'  <<-- FIX

    __NoxAST_NODE* right = __NoxParser_parse_logical_and(p);
    if (right == NULL)
      { __NoxParser_ast_free(left);
        return NULL; }

    __NoxAST_OPERATOR op = AST_OR;

    __NoxAST_NODE* node = (__NoxAST_NODE*)
      _NOX_ALLOC(sizeof(__NoxAST_NODE));
    if (node == NULL) {
      __NoxParser_ast_free(left);
      __NoxParser_ast_free(right);
      return NULL;
    }

    node->_type = AST_BINARY;
    node->data.binary.left = left;
    node->data.binary.op = op;
    node->data.binary.right = right;

    left = node;
  }
  return left;
}



/*
- Função: __NoxParser_parse_condition

- Parâmetros:
  @p        - Informações do parser

- Objetivo:
Analisar e construir nós AST para uma
condição

- Funcionamento:

- Consumo de tokens:
  - Tokens de __NoxParser_parse_comparison

- Retorno:
  * Ponteiro para o nó AST (__NoxAST_NODE*)
  ! NULL

- Notas:
  * Implementa associatividade à esquerda.
*/

// Destrutor para bloco de condição
static inline void
  __NoxParser_condition_destructor(void* element)
{
  if (element == NULL) return;

  __NoxParser_ast_free(*(__NoxAST_NODE**)element);
}

// Copier para bloco de condição
// Recebe @dest e @src como endereços de slots
// do tipo __NoxAST_NODE* (é o que __NoxDynaArr_push
// passa: (char*)data + n*element_size, com
// element_size == sizeof(__NoxAST_NODE*)).
static inline __NoxValue_BOOLEAN_TYPE
  __NoxParser_condition_copier(
    void* dest, const void* src
  ) {
  if (dest == NULL || src == NULL)
    { return __NoxBool_FALSE_VALUE; }

  __NoxAST_NODE* s = *(__NoxAST_NODE* const *)src;
  if (s == NULL) {
    *(__NoxAST_NODE**)dest = NULL;
    return __NoxBool_TRUE_VALUE;
  }

  __NoxAST_NODE* d = (__NoxAST_NODE*)
    _NOX_ALLOC(sizeof(__NoxAST_NODE));
  if (d == NULL)
    { return __NoxBool_FALSE_VALUE; }

  d->_type = s->_type;

  switch (s->_type) {
    case AST_LITERAL:
    case AST_IDENTIFIER:
    case AST_UNARY:
      d->data.token = s->data.token;
      break;

    case AST_BINARY:
      d->data.binary.op = s->data.binary.op;
      if (
        !__NoxParser_condition_copier(
          (void*)&d->data.binary.left,
          (void*)&s->data.binary.left
        ) ||
        !__NoxParser_condition_copier(
          (void*)&d->data.binary.right,
          (void*)&s->data.binary.right
        )
      ) {
        __NoxParser_ast_free(d);
        return __NoxBool_FALSE_VALUE;
      }
      break;

    case AST_ASSIGNMENT:
      d->data.assignment.type          = s->data.assignment.type;
      d->data.assignment.explicit_type = s->data.assignment.explicit_type;
      if (
        !__NoxParser_condition_copier(
          (void*)&d->data.assignment.identifier,
          (void*)&s->data.assignment.identifier
        ) ||
        !__NoxParser_condition_copier(
          (void*)&d->data.assignment.value,
          (void*)&s->data.assignment.value
        )
      ) {
        __NoxParser_ast_free(d);
        return __NoxBool_FALSE_VALUE;
      }
      break;

    case AST_IF: {
      if (
        !__NoxParser_condition_copier(
          (void*)&d->data.if_statement.condition,
          (void*)&s->data.if_statement.condition
        )
      ) {
        __NoxParser_ast_free(d);
        return __NoxBool_FALSE_VALUE;
      }

      d->data.if_statement.then_branch = NULL;
      d->data.if_statement.else_branch = NULL;

      if (s->data.if_statement.then_branch != NULL) {
        __NoxDynaArr__* tb = __NoxDynaArr_create(
          sizeof(__NoxAST_NODE*),
          __NoxParser_condition_copier,
          __NoxParser_condition_destructor,
          NULL
        );
        if (tb == NULL) {
          __NoxParser_ast_free(d);
          return __NoxBool_FALSE_VALUE;
        }

        for (size_t i = 0; i < s->data.if_statement.then_branch->count; i++) {
          void* src_slot = (char*)s->data.if_statement.then_branch->data
                           + i * s->data.if_statement.then_branch->element_size;
          __NoxAST_NODE* child = *(__NoxAST_NODE**)src_slot;

          if (!__NoxDynaArr_push(tb, &child, sizeof(__NoxAST_NODE*))) {
            __NoxDynaArr_free(tb);
            __NoxParser_ast_free(d);
            return __NoxBool_FALSE_VALUE;
          }
          /* array agora tem a CÓPIA; libera o original */
          __NoxParser_ast_free(child);
        }

        d->data.if_statement.then_branch = tb;
      }

      if (s->data.if_statement.else_branch != NULL) {
        __NoxDynaArr__* eb = __NoxDynaArr_create(
          sizeof(__NoxAST_NODE*),
          __NoxParser_condition_copier,
          __NoxParser_condition_destructor,
          NULL
        );
        if (eb == NULL) {
          if (d->data.if_statement.then_branch)
            __NoxDynaArr_free(d->data.if_statement.then_branch);
          __NoxParser_ast_free(d);
          return __NoxBool_FALSE_VALUE;
        }

        for (size_t i = 0; i < s->data.if_statement.else_branch->count; i++) {
          void* src_slot = (char*)s->data.if_statement.else_branch->data
                           + i * s->data.if_statement.else_branch->element_size;
          __NoxAST_NODE* child = *(__NoxAST_NODE**)src_slot;

          if (!__NoxDynaArr_push(eb, &child, sizeof(__NoxAST_NODE*))) {
            __NoxDynaArr_free(eb);
            if (d->data.if_statement.then_branch)
              __NoxDynaArr_free(d->data.if_statement.then_branch);
            __NoxParser_ast_free(d);
            return __NoxBool_FALSE_VALUE;
          }
          __NoxParser_ast_free(child);
        }

        d->data.if_statement.else_branch = eb;
      }
      break;
    }

    default:
      break;
  }

  *(__NoxAST_NODE**)dest = d;
  return __NoxBool_TRUE_VALUE;
}

static inline __NoxAST_NODE*
  __NoxParser_parse_condition(
    parser_info* p
  ) {
  if (!__NoxParser_is_parserinfo_valid(p))
    { return NULL; }

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "PARSER",
    "parse_condition",
    "iniciado\n"
  );

  _NOX_DEBUG_LOG(
    DEBUG_VERIFY,
    "PARSER",
    "parse_condition",
    "token atual: %s\n",
    __NoxDebug_show_token(__NoxParser_peek(p, 0))
  );
  if (!__NoxParser_match(p, IF, 0)) 
    { return NULL; }

  __NoxParser_advance(p, 0); // consome 'if'

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "PARSER",
    "parse_condition",
    "obtendo condição\n"
  );
  __NoxAST_NODE* condition =
    __NoxParser_parse_logical_or(p);
  if (condition == NULL) {
    _NOX_DEBUG_LOG(
      DEBUG_VERIFY,
      "PARSER",
      "parse_condition",
      "não foi possível obter condição\n"
    );

    const __NoxToken__* t =
      __NoxParser_peek(p, -1);

    if (t == NULL) 
      { return NULL; }

    if (!__NoxError_free(
        __NoxParser_create_error(
          p,
          ERROR_SYNTAX,
          t->line,
          t->column,
          t->column + t->length,
          "expected expression"
        )
      )) {
      _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "PARSER",
        "parse_condition",
        "Não foi possível liberar erro 'expected expression'\n"
      );
    }
    return NULL;
  }
  else {
    _NOX_DEBUG_LOG(
      DEBUG_VERIFY,
      "PARSER",
      "parse_condition",
      "condição obtida\n"
    );
  }

  // Pula linhas
  while (__NoxParser_match(p, NEW_LINE, 0))
    { __NoxParser_advance(p, 0); }
  // ---

  if (!__NoxParser_expect(
    p,
    LBRACE,
    "expected '{'"
  )) {
    __NoxParser_ast_free(condition);
    return NULL;
  }
  __NoxParser_advance(p, 0); // Consome '{'

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "PARSER",
    "parse_condition",
    "criando bloco 'then'\n"
  );
  __NoxDynaArr__* then_block = 
    __NoxDynaArr_create(
      sizeof(__NoxAST_NODE*),
      __NoxParser_condition_copier,
      __NoxParser_condition_destructor,
      NULL
    );

  if (then_block == NULL) {
    _NOX_DEBUG_LOG(
      DEBUG_ERROR,
      "PARSER",
      "parse_condition",
      "bloco 'then' = NULL\n"
    );

    const __NoxToken__* t =
      __NoxParser_peek(p, 0);

    if (t == NULL) {
      __NoxParser_ast_free(condition);
      return NULL;
    }

    __NoxError_free(
      __NoxParser_create_error(
        p,
        ERROR_SYNTAX,
        t->line,
        t->column,
        t->column + t->length,
        "expected expression after condition branch"
      )
    );
    __NoxParser_ast_free(condition);
    return NULL;
  }

  // parse do bloco 'then' até '}'
  while (!__NoxParser_match(p, RBRACE, 0))
  {
    if (__NoxParser_match(p, EOF_TOKEN, 0)) {
      const __NoxToken__* t =
        __NoxParser_peek(p, 0);
      if (t == NULL) {
        __NoxParser_ast_free(condition);
        __NoxDynaArr_free(then_block);
        return NULL;
      }

      if (!__NoxError_free(
        __NoxParser_create_error(
          p,
          ERROR_SYNTAX,
          t->line,
          t->column,
          t->column + t->length,
          "'{' not closed"
        )
      )) {
        __NoxParser_ast_free(condition);
        __NoxDynaArr_free(then_block);
      }

      return NULL;
    }

    // Pula linhas
    while (__NoxParser_match(p, NEW_LINE, 0))
      { __NoxParser_advance(p, 0); }
    // ---

    if (__NoxParser_match(p, RBRACE, 0)) break;

    __NoxAST_NODE* stmt = NULL;
    if (
      __NoxParser_match(p, IDENTIFIER, 0) &&
      (
        __NoxParser_match(p, ASSIGN, 1) ||
        __NoxParser_match(p, PIPE, 1)
      )
    ) { 
      _NOX_DEBUG_LOG(
        DEBUG_INFO,
        "PARSER",
        "parse_condition",
        "iniciando 'parse_statement'\n"
      );
      stmt = __NoxParser_parse_statement(p); 
    }

    else if (__NoxParser_match(p, LITERAL, 0) &&
             __NoxParser_peek(p, 0)->_literal_type == VALUE_INT_LITERAL)
    { 
      _NOX_DEBUG_LOG(
        DEBUG_INFO,
        "PARSER",
        "parse_condition",
        "iniciando 'parse_expression'\n"
      );
      stmt = __NoxParser_parse_expression(p); 
    }

    else if (__NoxParser_match(p, IF, 0)) {
      _NOX_DEBUG_LOG(
        DEBUG_INFO,
        "PARSER",
        "parse_condition",
        "iniciando 'parse_condition'\n"
      );

      stmt = __NoxParser_parse_condition(p); 
    }

    else {
      _NOX_DEBUG_LOG(
        DEBUG_INFO,
        "PARSER",
        "parse_condition",
        "iniciando 'parse_primary'\n"
      );
      stmt = __NoxParser_parse_primary(p); 
    }

    if (stmt == NULL) {
      __NoxParser_ast_free(condition);
      __NoxDynaArr_free(then_block);
      return NULL;
    }

    if (!__NoxDynaArr_push(then_block, &stmt, sizeof(__NoxAST_NODE*))) {
      __NoxParser_ast_free(stmt);
      __NoxParser_ast_free(condition);
      __NoxDynaArr_free(then_block);
      return NULL;
    }

    /* array copiou; libera o original */
    __NoxParser_ast_free(stmt);
  }

  __NoxParser_advance(p, 0); // consome '}'

  __NoxDynaArr__* else_block = NULL;

  // Pula linhas
  while (__NoxParser_match(p, NEW_LINE, 0))
    { __NoxParser_advance(p, 0); }
  // ---

  if (__NoxParser_match(p, ELSE, 0)) {
    __NoxParser_advance(p, 0); // consome 'else'

    if (__NoxParser_match(p, IF, 0)) {
      // Recursão: 'else if' — o if aninhado vira
      // um único elemento de else_block
      __NoxAST_NODE* nested_if = __NoxParser_parse_condition(p);
      if (nested_if == NULL) {
        _NOX_DEBUG_LOG(
          DEBUG_ERROR,
          "PARSER",
          "parse_condition",
          "nested_if = NULL\n"
        );
        __NoxParser_ast_free(condition);
        __NoxDynaArr_free(then_block);
        return NULL;
      }

      else_block = __NoxDynaArr_create(
        sizeof(__NoxAST_NODE*),
        __NoxParser_condition_copier,
        __NoxParser_condition_destructor,
        NULL
      );
      if (else_block == NULL) {
        __NoxParser_ast_free(nested_if);
        __NoxParser_ast_free(condition);
        __NoxDynaArr_free(then_block);
        return NULL;
      }

      if (!__NoxDynaArr_push(else_block, &nested_if, sizeof(__NoxAST_NODE*))) {
        __NoxParser_ast_free(nested_if);
        __NoxParser_ast_free(condition);
        __NoxDynaArr_free(then_block);
        __NoxDynaArr_free(else_block);
        return NULL;
      }
      __NoxParser_ast_free(nested_if);
    }
    else {
      // Pula linhas
      while (__NoxParser_match(p, NEW_LINE, 0))
        { __NoxParser_advance(p, 0); }
      // ---

      if (!__NoxParser_expect(
        p,
        LBRACE,
        "expected '{'")) {
        __NoxParser_ast_free(condition);
        __NoxDynaArr_free(then_block);
        return NULL;
      }
      __NoxParser_advance(p, 0); // consome '{'

      else_block = __NoxDynaArr_create(
        sizeof(__NoxAST_NODE*),
        __NoxParser_condition_copier,
        __NoxParser_condition_destructor,
        NULL
      );
      if (else_block == NULL) {
        _NOX_DEBUG_LOG(
          DEBUG_ERROR,
          "PARSER",
          "parse_condition",
          "erro ao criar 'else_block'"
        );

        __NoxParser_ast_free(condition);
        __NoxDynaArr_free(then_block);
        return NULL;
      }

      while (!__NoxParser_match(p, RBRACE, 0)) {
        if (__NoxParser_match(p, EOF_TOKEN, 0)) {
          const __NoxToken__* t =
            __NoxParser_peek(p, 0);
          if (t == NULL) {
            __NoxParser_ast_free(condition);
            __NoxDynaArr_free(then_block);
            __NoxDynaArr_free(else_block);
            return NULL;
          }

          if (!__NoxError_free(
            __NoxParser_create_error(
              p,
              ERROR_SYNTAX,
              t->line,
              t->column,
              t->column + t->length,
              "'{' not closed"
            )
          )) {
            __NoxParser_ast_free(condition);
            __NoxDynaArr_free(then_block);
            __NoxDynaArr_free(else_block);
          }

          return NULL;
        }

        // Pula linhas
        while (__NoxParser_match(p, NEW_LINE, 0))
          { __NoxParser_advance(p, 0); }
        // ---

        if (__NoxParser_match(p, RBRACE, 0)) { break; }

        __NoxAST_NODE* stmt = NULL;
        if (
          __NoxParser_match(p, IDENTIFIER, 0) &&
          (
            __NoxParser_match(p, ASSIGN, 1) ||
            __NoxParser_match(p, PIPE, 1)
          )
        ) { 
          _NOX_DEBUG_LOG(
            DEBUG_INFO,
            "PARSER",
            "parse_condition",
            "iniciando 'parse_statement'\n"
          );
          stmt = __NoxParser_parse_statement(p); 
        }

        else if (__NoxParser_match(p, LITERAL, 0) &&
                 __NoxParser_peek(p, 0)->_literal_type == VALUE_INT_LITERAL)
        { 
          _NOX_DEBUG_LOG(
            DEBUG_INFO,
            "PARSER",
            "parse_condition",
            "iniciando 'parse_expression'\n"
          );
          stmt = __NoxParser_parse_expression(p); 
        }

        else if (__NoxParser_match(p, IF, 0)) {
          _NOX_DEBUG_LOG(
            DEBUG_INFO,
            "PARSER",
            "parse_condition",
            "iniciando 'parse_condition'\n"
          );

          stmt = __NoxParser_parse_condition(p); 
        }

        else {
          _NOX_DEBUG_LOG(
            DEBUG_INFO,
            "PARSER",
            "parse_condition",
            "iniciando 'parse_primary'\n"
          );
          stmt = __NoxParser_parse_primary(p); 
        }

        if (stmt == NULL) {
          __NoxParser_ast_free(condition);
          __NoxDynaArr_free(then_block);
          __NoxDynaArr_free(else_block);
          return NULL;
        }

        if (!__NoxDynaArr_push(else_block, &stmt, sizeof(__NoxAST_NODE*))) {
          __NoxParser_ast_free(stmt);
          __NoxParser_ast_free(condition);
          __NoxDynaArr_free(then_block);
          __NoxDynaArr_free(else_block);
          return NULL;
        }

        /* array copiou; libera o original */
        __NoxParser_ast_free(stmt);
      }

      __NoxParser_advance(p, 0); // consome '}'
    }
  }

  // Monta nó AST_IF
  __NoxAST_NODE* node = (__NoxAST_NODE*)
    _NOX_ALLOC(sizeof(__NoxAST_NODE));
  if (node == NULL) {
    __NoxParser_ast_free(condition);
    __NoxDynaArr_free(then_block);
    if (else_block) __NoxDynaArr_free(else_block);
    return NULL;
  }

  node->_type = AST_IF;
  node->data.if_statement.condition   = condition;
  node->data.if_statement.then_branch = then_block;
  node->data.if_statement.else_branch = else_block; // pode ser NULL

  return node;
}

// =========================


/*
- Função: __NoxParser_parse_program

- Parâmetros:
  @p        - Informações do parser

- Objetivo:
Avaliar um programa por completo, gerando
nós AST a cada operação.

- Funcionamento:
1. Valida @p
  Se não: retorna NULL

2. Aloca o array nó AST principal em
$main_node
  Se NULL: retorna NULL

3. Cria o array de nós AST em $main_node->statements
  Se NULL: libera $main_node e retorna NULL

4. Inicia o loop principal com condição de
parada quando o token atual não existe ou
seu tipo é EOF_TOKEN

5. Pula tokens de tipo NEW_LINE

6. Cria um nó AST e identifica seu tipo
correspondente ao tipo do token atual,
chamando a função correta para cada caso.
  Se for NULL: libera $main_node e retorna NULL

7. Adiciona o nó AST ao nó principal $main_node
  Se não: libera $main_node->statements e $main_node
    e retorna NULL

8. Retorna $main_node

- Consumo de tokens:
  - Tokens de todas as outras funções de parsing

- Retorno:
  * Ponteiro para o nó AST de programa (array de nós AST, __NoxAST_PROGRAM*)
  ! NULL
*/
static inline void
  __NoxParser_program_destructor(void* element) {
  if (element == NULL) return;

  __NoxAST_NODE* node = *(__NoxAST_NODE**)element;
  __NoxParser_ast_free(node);
}

static inline __NoxAST_PROGRAM*
  __NoxParser_parse_program(parser_info* p) {
  if (!__NoxParser_is_parserinfo_valid(p))
    { _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "PARSER",
        "parse_program",
        "parser_info (p) is null\n"
      );
      return NULL; }

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "PARSER",
    "parse_program",
    "alocando 'main_node'"
  );
  __NoxAST_PROGRAM* main_node = (__NoxAST_PROGRAM*)\
      _NOX_ALLOC(sizeof(__NoxAST_PROGRAM));
  if (main_node == NULL)
    { _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "PARSER",
        "parse_program",
        "\nerro ao alocar 'main_node'\n"
      );
      return NULL; }

  _NOX_DEBUG_LOG(
    DEBUG_VERIFY,
    "","",
    " [DONE]\n"
  );
  main_node->statements = 
    __NoxDynaArr_create(sizeof(__NoxAST_NODE*),
    NULL,
    __NoxParser_program_destructor,
    NULL
  );

  if (main_node->statements == NULL)
    { _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "PARSER",
        "parse_program",
        "\nerro ao criar 'main_node->statements'\n"
      );
      _NOX_FREE(main_node);
      return NULL; }

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "PARSER",
    "parse_program",
    "iniciando loop principal\n"
  );
  while (
    __NoxParser_peek(p, 0) != NULL &&
    __NoxParser_peek(p, 0)->_type != EOF_TOKEN
  ) {
    char token[128];
    snprintf(
      token, sizeof(token),
      "%.*s",
      (int)__NoxParser_peek(p, 0)->length, 
      __NoxParser_peek(p, 0)->start
    );
    _NOX_DEBUG_LOG(
      DEBUG_VERIFY,
      "PARSER",
      "parse_program",
      "Index: %zu | Token: %s | Texto: '%s'\n",
      p->pos,
      __NoxDebug_show_token(__NoxParser_peek(p, 0)),
      (token[0] == '\n' ? "\\n" : token)
    );
    if (__NoxParser_match(p, NEW_LINE, 0)) {
      __NoxParser_advance(p, 0);
      continue;
    }

    __NoxAST_NODE* node = NULL;
    if (
      __NoxParser_match(p, IDENTIFIER, 0) &&
      (
        __NoxParser_match(p, ASSIGN, 1) ||
        __NoxParser_match(p, PIPE, 1)
      )
    ) { 
      _NOX_DEBUG_LOG(
        DEBUG_INFO,
        "PARSER",
        "parse_program",
        "iniciando 'parse_statement'\n"
      );
      node = __NoxParser_parse_statement(p); 
    }

    else if (p->tokens[p->pos]._type == LITERAL &&
    p->tokens[p->pos]._literal_type == VALUE_INT_LITERAL)
    { 
      _NOX_DEBUG_LOG(
        DEBUG_INFO,
        "PARSER",
        "parse_program",
        "iniciando 'parse_expression'\n"
      );
      node = __NoxParser_parse_expression(p); 
    }

    else if (p->tokens[p->pos]._type == IF) {
      _NOX_DEBUG_LOG(
        DEBUG_INFO,
        "PARSER",
        "parse_program",
        "iniciando 'parse_condition'\n"
      );

      node = __NoxParser_parse_condition(p); 
    }

    else {
      _NOX_DEBUG_LOG(
        DEBUG_INFO,
        "PARSER",
        "parse_program",
        "iniciando 'parse_primary'\n"
      );
      node = __NoxParser_parse_primary(p); 
    }
    
    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "PARSER",
      "parse_program",
      "expression: %s | pos=%zu\n",
      node ? "OK" : "NULL",
      p->pos
    );
    if (node == NULL)
      { _NOX_DEBUG_LOG(
          DEBUG_ERROR,
          "PARSER",
          "parse_program",
          "node = null\n"
        );
        __NoxDynaArr_free(main_node->statements);
        _NOX_FREE(main_node);
        return NULL; }

    if (!__NoxDynaArr_push(main_node->statements, &node, sizeof(__NoxAST_NODE*)))
      { _NOX_DEBUG_LOG(
          DEBUG_ERROR,
          "PARSER",
          "parse_program",
          "erro ao adicionar 'node' em 'main_node->statements'\n"
        );
        __NoxDynaArr_free(main_node->statements);
        __NoxParser_ast_free(node);
        _NOX_FREE(main_node);
        return NULL; }
  }

  // DEBUG FINAL
  __NoxDebug_show_program(main_node);
  return main_node;
}
// =======================