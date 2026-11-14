# Guia de Documentação do Nox

## 1. Introdução
Este arquivo apresenta uma breve explicação sobre
como funciona a documentação interna do código-fonte
implementado em *C* da linguagem **Nox**.
Seu principal objetivo é facilitar a compreensão 
humana e permitir que agentes de Inteligência Artificial 
compreendam com precisão a arquitetura, o fluxo de 
ponteiros e a lógica de execução.

## 2. Notação
A documentação interna utiliza de certos símbolos para
identificação de alguns itens específicos, como parâmetros
e variáveis.

### 2.1. Parâmetros (@)
As funções documentadas possuem os parâmetros representados
pelo símbolo **@** como prefixo para facilitar a identificação
dos parâmetros.
- Exemplos retirados de [Parser](../parser/parser.h):
  ```1. Valida @p```
  ```3. Retorna o token definido por @n```
  ```2. Verifica se @n [...]```

### 2.2. Variáveis Internas ($)
Além dos parâmetros com *@*, também existem as variáveis
internas, onde o prefixo é **$** com a intenção de diferenciar
as variáveis de outros itens.
- Exemplos retirados de [Parser](../parser/parser.h):
  ```4. Compara o tipo de $current [...]```
  ```3. Guarda o token atual em $token [...]```

### 2.3. Referências Diretas
As referências diretas utilizadas durante a documentação,
seja funções, estruturas ou outros elementos, são escritas
com seu nome real, sem modificação.
- Exemplos retirados de [Parser](../parser/parser.h):
  ```6. Retorna __NoxBool_FALSE_VALUE.```
  ```2. Captura o token atual com __NoxParser_peek [...]```


## 3. Estrutura dos Documentos
O código-fonte da linguagem **Nox** apresenta diferentes 
formas de documentações, mas também possui um pequeno
padrão em sua estrutura, seguindo uma determinada sequência:
- 1. Identificação da função
- 2. Parâmetros
- 3. Objetivo
- 4. Funcionamento
- 5. Retorno
- 6. Notas

**NOTA:** *Nem todas as documentações apresentam exatamente
este padrão!*

### 3.1. Identificação da função
A identificação da função é representada pelo trecho
"- Função: ", que apresenta o nome real da função.
- Exemplos retirado de [Parser](../parser/parser.h):
  ```- Função: __NoxParser_match"```
  ```- Função: __NoxParser_advance```
  ```- Função: __NoxParser_parse_statement```

### 3.2. Parâmetros
Os parâmetros de cada função são mostrados como uma
lista, iniciada por "- Parâmetros:".
Cada parâmetro apresenta um breve resumo sobre o que
significa ou o que faz.
- Exemplo retirado de [Parser](../parser/parser.h):
  ```
  - Parâmetros:
  @p        - Informações do parser
  @n        - Deslocamento
  ```

### 3.3. Objetivo
Além do identificador e dos parâmetros, também possui
o objetivo da função, apresentado pelo trecho "- Objetivo:",
representando um breve resumo sobre o que a função deve
fazer.
- Exemplo retirado de [Parser](../parser/parser.h):
  ```
  - Objetivo:
  Retornar um ponteiro para o token 
  correspondente à posição definida 
  por @n
  ```

### 3.4. Funcionamento
Sendo crucial na documentação, o trecho "- Funcionamento:"
mostra o passo a passo numerado de como a função deve se 
comportar e como ela lida com cada operação.
- Exemplo retirado de [Parser](../parser/parser.h):
  ```
  - Funcionamento:
  1. Verifica se @p é válido
    Se não: retorna NULL

  2. Verifica se o deslocamento é
  válido
    Se não: retorna NULL

  3. Retorna o token definido por @n
  ```

### 3.5. Retorno
Mais propriamente dito, representa o que a função
retorna em todos os casos no trecho "- Retorno:".
Essa seção em específico possui um padrão, onde
para representar:
  - sucesso: utiliza '*'
  - falha ou erro: utiliza '!'
antes do tipo especificado.

- Exemplo retirado de [Parser](../parser/parser.h):
  ```
  - Retorno: 
    * Ponteiro para o nó AST (__NoxAST_NODE*)
    ! NULL
  ```

### 3.6. Notas
Essa seção, do trecho "- Notas:", registra 
particularidades, limitações, decisões e efeitos
importantes que não ficaram evidentes nas outras
seções, apresentados em tópicos.
- Exemplo retirado de [Parser](../parser/parser.h):
  ```
  - Notas:
    * Implementa associatividade à esquerda.
    * Possui precendência menor que os fatores
      (multiplicação e divisão)
  ```

### 3.7. Exemplo de um documento
Para melhor compreensão, vai um exemplo retirado de 
[Parser](../parser/parser.h):
```
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

  - Notas:
    * Não avança posição do parser
```
