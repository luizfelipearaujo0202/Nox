#define _GNU_SOURCE // permitir strtok_r
// main.c

#include <string.h>
#include <stdio.h>

#include "error/error_registry.h"
#include "parser/var/var_table.h"

#include "structures/exec_structures.h"
#include "structures/ast_structures.h"
#include "structures/structures.h"
#include "token_array.h"

#include "file_ctrl.h"

#include "parser/parser.h"
#include "lexer.h"
#include "executor.h"

#include "error.h"
#include "debug.h"

unsigned int _NOX_DEBUG_ENABLED = 0;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Uso: %s <arquivo.nox>\n", argv[0]);
    return 1;
  }

  if (argc <= 3) {
    for (int i = 1; i < argc; i++) {
      if (strncmp(argv[i], "-debug=", 7) == 0)
      {
        char* saveptr = NULL;
        char* token = strtok_r(argv[i] + 7, ",", &saveptr);
        while (token != NULL) {
          if (strcmp(token, "all") == 0)
            { _NOX_DEBUG_ENABLED = DEBUG_ALL; }
          else if (strcmp(token, "none") == 0)
            { _NOX_DEBUG_ENABLED = DEBUG_NONE; }
          else if (strcmp(token, "verify") == 0)
            { _NOX_DEBUG_ENABLED = DEBUG_VERIFY; }
          else if (strcmp(token, "info") == 0)
            { _NOX_DEBUG_ENABLED = DEBUG_INFO; }
          else if (strcmp(token, "error") == 0)
            { _NOX_DEBUG_ENABLED = DEBUG_ERROR; }

          else if (strcmp(token, "help") == 0)
          {
            printf("Available commands to 'debug':\n");
            printf("  1. all: show all\n");
            printf("  2. none: silent debug\n");
            printf("  3. verify: show only 'verify' messages\n");
            printf("  4. info: show only 'info' messages\n");
            printf("  5. error: show only 'error' messages\n");
            printf("Can be combined like '-debug=info,verify''\n");
            return 1;
          }
          else {
            printf("'-debug=': invalid flag; type -debug=help to get available commands\n");
            return 1;
          }

          token = strtok_r(NULL, ",", &saveptr);
        }
      }
    }
  }

  char* src = read_file(argv[1]);

  if (src == NULL) {
    fprintf(stderr, "Erro ao ler o arquivo.\n");
    return 1;
  }

  __NoxError_context context = {__NoxError_create_regarray()};

  if (context.registry == NULL)
    { printf("Registry array alloc failed\n"); 
      free(src);
      return 1; }

  __NoxLexer_pos lexer = {
      src,
      1, 1,
      0,
      &context
  };

  __NoxToken_array arr = {0};

  for (;;) {
    __NoxToken__ t = __NoxLexer_next(&lexer);

    if (!__NoxToken_array_push(&arr, t)) {
      fprintf(stderr, "Erro ao alocar tokens.\n");
      return 1;
    }

    if (t._type == EOF_TOKEN) 
      { break; }
  }

  __NoxError_registration* err = 
      __NoxError_get_first(context.registry);

  if (err != NULL) {
    __NoxError_source* source =
        __NoxError_create_source(src, err->error->line);

    if (source == NULL)
      { __NoxError_free_regarray(context.registry);
        __NoxToken_array_free(&arr);
        free(src);
        return 1; }

    __NoxError_render_line(err->error, source);
    __NoxError_free_source(source);
    __NoxError_free_regarray(context.registry);
    __NoxToken_array_free(&arr);
    
    free(src);
    return 1;
  }

  parser_info parser = {
    .tokens = arr.data,
    .count = arr.count,
    .pos = 0,
    .errors = &context
  };

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "MAIN",
    "",
    "Total de tokens: %zu\n\n", arr.count
  );

  for (size_t i = 0; i < arr.count; i++) {
    char token[128];
    snprintf(
      token, sizeof(token),
      "%.*s",
      (int)arr.data[i].length, 
      arr.data[i].start
    );
    _NOX_DEBUG_LOG(
      DEBUG_INFO,
      "MAIN",
      "",
      "Token: %s | Texto: '%s' | Tamanho: %zu\n",
      show_token(&arr.data[i]),
      (token[0] == '\n' ? "\\n" : token),
      arr.data[i].length
    );
  }

  __NoxAST_PROGRAM* pb = __NoxParser_parse_program(&parser);
  if (pb == NULL)
    { _NOX_DEBUG_LOG(
        DEBUG_ERROR,
        "MAIN",
        "",
        "'pb' = null\n"
      );
      err = __NoxError_get_first_fatal(context.registry);
      if (err != NULL) {
        __NoxError_source* source =
            __NoxError_create_source(src, err->error->line);

        if (source == NULL)
          { __NoxError_free_regarray(context.registry);
            __NoxToken_array_free(&arr);
            free(src);
            return 1; }

        __NoxError_render_line(err->error, source);
        __NoxError_free_source(source);
        __NoxError_free_regarray(context.registry);
        __NoxToken_array_free(&arr);
        free(src);
        return 1;
      }

      __NoxToken_array_free(&arr);
      __NoxError_free_regarray(context.registry);
      free(src);
      return 1; }

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "MAIN",
    "",
    "Criando HashTable de variáveis\n"
  );
  // Mounting execution items
  __NoxVar_HashTable* hashtable =
      __NoxVar_HashTable_create(4, 3, 10);
  if (hashtable == NULL) {
    _NOX_DEBUG_LOG(
      DEBUG_ERROR,
      "MAIN",
      "",
      "HashTable não pôde ser alocada\n"
    );

    __NoxToken_array_free(&arr);
    __NoxError_free_regarray(context.registry);
    __NoxParser_program_free(pb);
    __NoxFile_free(src);
    return 1;
  }

  // Execution context mounting
  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "MAIN",
    "",
    "Montando contexto do Executor\n"
  );
  __NoxExec_context exec_context;
  exec_context.program = pb;
  exec_context.index = 0;
  exec_context.node = (__NoxAST_NODE*)\
    __NoxDynaArr_get(pb->statements, 0);
  exec_context.errors = &context;
  exec_context.table = hashtable;

  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "MAIN",
    "",
    "Executando\n"
  );
  if (!__NoxExec_process_single(&exec_context, 0)) {
    _NOX_DEBUG_LOG(
      DEBUG_ERROR,
      "MAIN",
      "",
      "A execução falhou\n"
    );

    __NoxVar_HashTable_free(hashtable);
    __NoxToken_array_free(&arr);
    __NoxError_free_regarray(context.registry);
    __NoxParser_program_free(pb);
    __NoxFile_free(src);
    return 1;
  }
  _NOX_DEBUG_LOG(
    DEBUG_INFO,
    "MAIN",
    "",
    "done.\n"
  );

  __NoxParser_program_free(pb);
  __NoxVar_HashTable_free(hashtable);
  __NoxToken_array_free(&arr);
  __NoxFile_free(src);

  __NoxError_free_regarray(context.registry);
  return 0;
}