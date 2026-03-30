# Analisador Léxico para Linguagem FCC

Este projeto consiste em um **Analisador Léxico (Lexer)** desenvolvido em C++ para a disciplina de Compiladores. O objetivo é transformar um código-fonte de entrada em uma sequência de tokens classificados, gerando um relatório detalhado em formato JSON.

## 🚀 Funcionalidades

  - **Tokenização Completa**: Identifica palavras-chave, identificadores, números literais, strings e operadores.
  - **Suporte a Comentários**: Ignora corretamente comentários de linha (`//`) e blocos de múltiplas linhas (`/* ... */`).
  - **Tratamento de Strings**: Suporta aspas escapadas (`\"`) dentro de strings literais.
  - **Interface de Linha de Comando (CLI)**: Suporte a flags para modo verbose e definição de arquivo de saída.
  - **Diagnóstico de Erros**: Aponta a localização exata (linha e coluna) de erros léxicos com indicação visual no console.
  - **Exportação JSON**: Gera um arquivo estruturado com todos os tokens processados para integração com analisadores sintáticos.

## 🛠 Decisões de Implementação

  * **Namespaces Anônimos**: Utilizados para isolar funções auxiliares e padrões de Regex, evitando poluição do escopo global e conflitos de ligação (*linkage*).
  * **Variáveis Static**: Os objetos `std::regex` são declarados como estáticos dentro das funções de classificação. Isso garante que a "compilação" do autômato da Regex ocorra apenas uma vez, otimizando drasticamente a velocidade de processamento.
  * **Lookahead (Olhar à frente)**: Implementação manual de verificação de caracteres subsequentes para distinguir operadores simples (`>`) de compostos (`>=`).
  * **Gerenciamento de Estado**: Uso de flags de estado para tratar o contexto de comentários de bloco que cruzam múltiplas linhas.

## 📋 Pré-requisitos

  - Compilador C++ compatível com o padrão **C++17** ou superior (GCC, Clang ou MSVC).
  - Biblioteca `nlohmann/json` (incluída no projeto ou via gerenciador de pacotes).

## 🛠 Compilação e Build

Este projeto utiliza **CMake** para gerenciar as dependências e o processo de build. Para facilitar a configuração, incluímos um arquivo `CMakePresets.json`.

### Requisitos
* **CMake** 3.19 ou superior.
* **Compilador C++** com suporte a C++17 (GCC 9+, Clang 10+ ou MSVC 2019+).

### Usando CMake Presets (Recomendado)
Se o seu ambiente suporta Presets (como VS Code, CLion ou CMake via CLI 3.19+), você pode configurar e compilar com:

```bash
# Configurar o projeto usando o preset padrão
cmake --preset default

# Executar o build
cmake --build --build/default
```

### Compilação Manual (Caso não use Presets)
```bash
mkdir build && cd build
cmake ..
make
```

O executável gerado será o `fcc` (ou `fcc.exe` no Windows), localizado dentro da pasta `build`.

## 📖 Como Usar

A execução básica requer um arquivo de entrada:

```bash
./fcc arquivo_fonte.fcc
```

### Flags Disponíveis:

| Flag | Descrição |
| :--- | :--- |
| `-v`, `--verbose` | Exibe a lista de tokens detalhada diretamente no console. |
| `-o <nome.json>` | Define o nome do arquivo JSON de saída (padrão: `output.json`). |

**Exemplo Completo:**

```bash
./fcc_lexer exemplo.fcc --verbose -o tokens.json
```

## 🔍 Exemplo de Saída (Erro Léxico)

Caso o Lexer encontre um caractere inválido, ele exibirá:

```text
Erro: na linha 5, coluna 12
int valor = @50;
            ^
```

## 📁 Estrutura do JSON de Saída

O arquivo gerado segue este formato:

```json
[
  {
    "type": "KEYWORD",
    "lexeme": "int",
    "line": 1,
    "column": 1
  },
  {
    "type": "IDENTIFIER",
    "lexeme": "soma",
    "line": 1,
    "column": 5
  }
]
```

## ✒️ Autor

  * **Caio Reis** - [FromCaio](https://github.com/fromcaio)