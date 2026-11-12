#pragma once

// parser.h

#include "../structures/ast_structures.h"
#include "../structures/structures.h"
#include "../error/error_registry.h"
#include "../error.h"
#include "../debug.h"

#include <stddef.h>
#include <stdlib.h>
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
    c_end
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
    if (n->_type == AST_BINARY) {
      __NoxParser_ast_free(n->data.binary.left);
      __NoxParser_ast_free(n->data.binary.right);
    }
    else if (n->_type == AST_ASSIGNMENT) {
      __NoxParser_ast_free(n->data.assignment.identifier);
      __NoxParser_ast_free(n->data.assignment.value);
    }
    free(n);
}


static inline void
  __NoxParser_program_free(__NoxAST_PROGRAM* p)
{
  if (p == NULL) return;
  __NoxDynaArr_free(p->statements);
  free(p);
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
        malloc(sizeof(__NoxAST_NODE));

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
      free(node);
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
            "node = NULL"
          );
          return NULL; }

      current = __NoxParser_peek(p, 0);
      if (current == NULL) 
      { _NOX_DEBUG_LOG(
            DEBUG_ERROR,
            "PARSER",
            "parse_primary",
            "current = NULL"
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
          "current entrou no if de parênteses"
        );
        __NoxParser_ast_free(node);
        return NULL;
      }
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
      free(node); return NULL; 
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
          show_token(__NoxParser_peek(p, -1)),
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

    while(__NoxParser_is_operator(current, 0, 0, 1, 1, 0)) {
      __NoxAST_OPERATOR op;
      if (__NoxParser_is_operator(current, 0, 0, 1, 0, 0))
      { op = AST_MULTIPLY; }
      else op = AST_DIVIDE;

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
        (__NoxAST_NODE*)malloc(sizeof(__NoxAST_NODE));
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
        (__NoxAST_NODE*)malloc(sizeof(__NoxAST_NODE));
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

  if (!__NoxParser_match(p, IDENTIFIER, 0) &&
    !__NoxParser_expect(
    p,
    ASSIGN,
    "unexpected assignment"
  )) {
    _NOX_DEBUG_LOG(
      DEBUG_VERIFY,
      "PARSER",
      "parse_statement",
      "erro de '=' inesperado"
    );
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
    malloc(sizeof(__NoxAST_NODE));
  if (node == NULL) {
    __NoxParser_ast_free(identifier);
    __NoxParser_ast_free(value);
    return NULL; 
  }

  node->_type = AST_ASSIGNMENT;
  node->data.assignment.identifier = identifier;
  node->data.assignment.value = value;
  return node;
}


static inline void
  __NoxParser_program_destructor(void* element) {
  __NoxAST_NODE* node = (__NoxAST_NODE*)element;
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
      malloc(sizeof(__NoxAST_PROGRAM));
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
      free(main_node);
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
      show_token(__NoxParser_peek(p, 0)),
      (token[0] == '\n' ? "\\n" : token)
    );
    if (__NoxParser_match(p, NEW_LINE, 0)) {
      __NoxParser_advance(p, 0);
      continue;
    }

    __NoxAST_NODE* node = NULL;
    if (
      __NoxParser_match(p, IDENTIFIER, 0) &&
      __NoxParser_match(p, ASSIGN, 1)
    ) { _NOX_DEBUG_LOG(
          DEBUG_INFO,
          "PARSER",
          "parse_program",
          "iniciando 'parse_statement'\n"
        );
        node = __NoxParser_parse_statement(p); }

    else if (p->tokens[p->pos]._type == LITERAL &&
    p->tokens[p->pos]._literal_type == VALUE_INT_LITERAL)
      { _NOX_DEBUG_LOG(
          DEBUG_INFO,
          "PARSER",
          "parse_program",
          "iniciando 'parse_expression'\n"
        );
        node = __NoxParser_parse_expression(p); }

    else { _NOX_DEBUG_LOG(
            DEBUG_INFO,
            "PARSER",
            "parse_program",
            "iniciando 'parse_primary'\n"
          );
          node = __NoxParser_parse_primary(p); }
    
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
        free(main_node);
        return NULL; }

    if (!__NoxDynaArr_push(main_node->statements, node, sizeof(__NoxAST_NODE*)))
      { _NOX_DEBUG_LOG(
          DEBUG_ERROR,
          "PARSER",
          "parse_program",
          "erro ao adicionar 'node' em 'main_node->statements'\n"
        );
        __NoxDynaArr_free(main_node->statements);
        __NoxParser_ast_free(node);
        free(main_node);
        return NULL; }
  }

  return main_node;
}
// =======================