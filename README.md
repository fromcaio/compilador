# Compilador FCC

Compilador para uma versão simplificada da linguagem C, desenvolvido em C++ para a disciplina de Compiladores. Implementa as três fases de análise — léxica, sintática e semântica — de forma modular e sem uso de orientação a objetos.

## Fases Implementadas

### 1. Analisador Léxico (`lexical_analyser`)

Transforma o código-fonte em uma sequência de tokens classificados.

- Reconhece palavras-chave, identificadores, literais numéricos, strings e operadores
- Suporte a comentários de linha (`//`) e de bloco (`/* ... */`)
- Lookahead manual para operadores compostos (`>=`, `++`, `+=`, etc.)
- Detecção de erros léxicos com localização exata (linha e coluna):
  - `LEXICAL_ERROR_INVALID_CHAR` — símbolo fora do alfabeto da linguagem
  - `LEXICAL_ERROR_MALFORMED_NUMBER` — constante numérica inválida (ex: `10.5.2`, `123abc`)
  - `LEXICAL_ERROR_UNCLOSED_STRING` — string sem fechamento na mesma linha
  - `LEXICAL_ERROR_UNCLOSED_COMMENT` — bloco de comentário não encerrado
- Exportação dos tokens para JSON via flag `-l`

### 2. Analisador Sintático (`syntactic_analyser`)

Parser recursivo descendente (LL(1)) que valida a estrutura gramatical do programa.

- Reconhece declarações globais (variáveis e funções), blocos, comandos e expressões
- Precedência de operadores implementada via hierarquia de funções (`parse_AddExpr`, `parse_MultExpr`, etc.)
- Recuperação de erros em modo pânico (`synchronize`) para continuar a análise após falhas
- Suporte a: `if/else`, `while`, `for`, `return`, `break`, `continue`, arrays e chamadas de função

### 3. Analisador Semântico (`semantic_analyser`)

Executa verificações de significado integradas ao parser, sem construção de AST.

- **Tabela de símbolos** com nome, tipo, categoria (variável/função), escopo e linha de declaração
- **Gerenciamento de escopos** aninhados com campo `active` — símbolos são desativados ao sair do escopo e mantidos na tabela para impressão posterior
- Erros detectados:
  - Variável não declarada
  - Função não declarada
  - Redeclaração no mesmo escopo
  - Tipo incompatível em inicialização, atribuição ou operação aritmética/lógica
  - Retorno de valor em função `void`
  - Tipo de retorno incompatível com a declaração da função
  - Índice de array de tipo diferente de `int`

## Gramática Suportada

```
Program      → GlobalDecl*
GlobalDecl   → Type IDENTIFIER (FuncDeclTail | VarDeclTail)
FuncDeclTail → '(' Params ')' Block
VarDeclTail  → ('=' Expr | '[' NUMBER ']')? ';'
Params       → (Param (',' Param)*)?
Param        → Type IDENTIFIER
Block        → '{' Statement* '}'
Statement    → VarDeclStmt | IfStmt | WhileStmt | ForStmt
             | ReturnStmt | BreakStmt | ContinueStmt | Block | ExprStmt
Expr         → LogicalOr (AssignOp Expr)?
```

Expressões seguem precedência padrão: atribuição < lógico OR < lógico AND < igualdade < relacional < adição < multiplicação < unário < primário/postfix.

## Pré-requisitos

- Compilador C++ com suporte a **C++17** (GCC 9+, Clang 10+ ou MSVC 2019+)
- **CMake 3.14** ou superior
- A biblioteca `nlohmann/json` é baixada automaticamente via `FetchContent`

## Compilação

```bash
# Configurar (apenas na primeira vez)
cmake --preset default

# Compilar
cmake --build build/default
```

O executável gerado é `fcc` (ou `fcc.exe` no Windows) dentro de `build/default/`.

## Uso

```bash
fcc <entrada.c | entrada.json> [opções]
```

### Flags

| Flag | Descrição |
| :--- | :--- |
| `-v`, `--verbose` | Saída detalhada de todas as fases + tabela de símbolos |
| `--vlex` | Saída detalhada apenas da análise léxica |
| `--vsyn` | Trace das regras gramaticais durante a análise sintática |
| `-s`, `--sym` | Imprime a tabela de símbolos ao final |
| `-l` | Executa apenas a análise léxica e salva os tokens em JSON |
| `-o <arquivo>` | Nome do arquivo de saída para tokens (padrão: `output.json`) |
| `-h`, `--help` | Exibe a ajuda |

### Exemplos

```bash
# Análise completa silenciosa
fcc programa.c

# Análise com tabela de símbolos
fcc programa.c --sym

# Análise completa com saída detalhada
fcc programa.c -v

# Apenas tokenização, salva em arquivo
fcc programa.c -l -o tokens.json

# Retomar análise sintática a partir de tokens salvos
fcc tokens.json --sym
```

## Exemplos de Saída

### Análise bem-sucedida com tabela de símbolos

```
$ fcc programa.c --sym
>>> Sucesso: Analise concluida sem erros!

=== TABELA DE SIMBOLOS ===
Nome              Tipo      Escopo  Linha  Categoria
-----------------------------------------------------
globalVar         int       0       1      VARIAVEL
values            int[]     0       2      VARIAVEL
sum               int       0       4      FUNCAO
a                 int       1       4      VARIAVEL
b                 int       1       4      VARIAVEL
result            int       1       5      VARIAVEL
main              void      0       13     FUNCAO
i                 int       1       14     VARIAVEL
```

### Erro léxico

```
$ fcc programa.c --vlex
Erro: LEXICAL_ERROR_MALFORMED_NUMBER na linha 5, coluna 12
int valor = 10.5.2;
            ^
```

### Erros semânticos

```
$ fcc programa.c
Erro Semantico (linha 4): Variavel nao declarada: 'x'
Erro Semantico (linha 7): Tipo incompativel na atribuicao: 'int' e 'float'
>>> Falha: A analise semantica encontrou erros.
```

## Estrutura do Projeto

```
compilador/
├── CMakeLists.txt
├── src/
│   ├── HeaderFiles/
│   │   ├── lexical_analyser.h      # Tokens, tabela estática, regex
│   │   ├── semantic_analyser.h     # Struct Symbol, tabela de símbolos, API
│   │   └── syntactic_analyser.h    # Declarações do parser
│   └── SourceFiles/
│       ├── main.cpp                # CLI e orquestração das fases
│       ├── lexical_analyser.cpp    # Motor de tokenização
│       ├── semantic_analyser.cpp   # Tabela de símbolos e verificações
│       └── syntactic_analyser.cpp  # Parser + ações semânticas inline
└── entradas/
    ├── 01-entrada-valida.c         # Programa sintatica e semanticamente correto
    ├── E01–E11-*.c                 # Casos de erro sintático
    └── S01–S07-*.c                 # Casos de erro semântico
```

## Decisões de Implementação

- **Sem AST**: as verificações semânticas são executadas inline durante o parsing, eliminando a necessidade de uma árvore intermediária
- **Propagação de tipos via retorno**: funções de expressão retornam `TokenType::Type`, permitindo verificação de compatibilidade sem estado adicional
- **`TYPE_UNRESOLVED`**: sentinela (`EOF_TOKEN`) que suprime erros em cascata quando um tipo não pode ser determinado
- **Campo `active` na tabela de símbolos**: ao sair de um escopo, símbolos são desativados em vez de removidos — a tabela completa fica disponível para impressão ao final
- **`main` como identificador comum**: não é palavra-chave obrigatória; qualquer função pode servir como ponto de entrada

## Autor

**Caio Reis** — [fromcaio](https://github.com/fromcaio)
