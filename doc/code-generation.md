# Revisão: Fase de Geração de Código

Este documento explica como a geração de código assembly RISC-V foi implementada no
compilador FCC, com foco na arquitetura da solução — não repete a listagem de instruções
(ver [`doc/assembly-guide.md`](assembly-guide.md) para isso).

## Contexto: sem AST

O compilador não constrói uma árvore sintática abstrata. As três fases — léxica, sintática
e semântica — já operavam de forma totalmente inline dentro do parser recursivo descendente
(`syntactic_analyser.cpp`). A geração de código segue a mesma filosofia: **cada função de
parsing emite assembly no momento em que reconhece a construção correspondente**, sem uma
passagem posterior sobre uma árvore.

Isso só é possível porque a ordem de chamada das funções recursivas já reflete a precedência
e a estrutura da linguagem — a própria pilha de chamadas C++ faz o papel da árvore.

---

## `ExprResult`: o valor que substitui a árvore

Antes da geração de código, as funções de expressão (`parse_Expr`, `parse_AddExpr`, etc.)
retornavam apenas `TokenType::Type`, usado só para checagem de tipos. Para gerar código, cada
expressão também precisa informar **onde está o valor calculado**. A solução foi estender o
retorno para um struct:

```cpp
// src/HeaderFiles/code_generator.h
struct ExprResult {
    TokenType::Type type = TYPE_UNRESOLVED;  // usado pela checagem semântica (já existia)
    std::string reg;                          // registrador que contém o valor (novo)
};
```

Todas as assinaturas de expressão em [`syntactic_analyser.h`](../src/HeaderFiles/syntactic_analyser.h)
foram atualizadas de `TokenType::Type` para `ExprResult`. Cada função de expressão em
[`syntactic_analyser.cpp`](../src/SourceFiles/syntactic_analyser.cpp) faz três coisas ao
mesmo tempo:

1. Reconhece a gramática (como antes)
2. Verifica tipos usando `.type` (a lógica semântica já existente, inalterada)
3. Emite instruções e propaga `.reg` (a parte nova)

Exemplo em `parse_AddTail` (linha 821):
```cpp
ExprResult right = parse_MultExpr();
// (1) checagem semântica, igual a antes:
if (!is_unresolved(left.type) && !is_unresolved(right.type) && left.type != right.type)
    semantic_report_error(...);
// (2) geração de código, apenas se codegen_enabled:
std::string res = codegen_alloc_ireg(); // ou freg, se float
codegen_emit("    add  " + res + ", " + left.reg + ", " + right.reg);
codegen_free_reg(left.reg);
codegen_free_reg(right.reg);
return parse_AddTail({res_type, res});
```

Toda emissão é condicionada a `codegen_enabled` (ativado pela flag `-c` em `main.cpp`), então
rodar sem `-c` tem custo zero de geração de código — apenas a análise semântica de sempre.

---

## Alocação de registradores: pool simples, sem análise de vida

O gerador usa dois pools fixos, sem análise de liveness nem spill para a pilha:

```cpp
inline bool codegen_ireg_used[7] = {};  // t0–t6
inline bool codegen_freg_used[7] = {};  // ft0–ft6
```

`codegen_alloc_ireg()` / `codegen_alloc_freg()` varrem o array e devolvem o primeiro
registrador livre; `codegen_free_reg()` o libera. Cada função de expressão libera os
registradores dos operandos assim que produz o resultado — por isso a maioria das expressões
usa poucos registradores mesmo em expressões longas.

**Limitação aceita conscientemente**: como não há spill, uma expressão que precise de mais de
7 temporários inteiros simultâneos esgota o pool (lança `std::runtime_error`). Na prática,
isso não ocorre em expressões razoáveis (aritmética, comparações, chamadas simples).

**Limitação não intencional**: os registradores `t0`–`t6` são "caller-saved" pela convenção
RISC-V, mas o gerador não insere código de salvamento/restauração ao redor de `jal`. Uma
expressão que mantenha um temporário vivo através de **duas chamadas de função** (por
exemplo, `fib(n-1) + fib(n-2)`, onde o resultado da primeira chamada precisa sobreviver à
segunda) pode ter esse valor sobrescrito pela chamada recursiva. Isso é uma dívida técnica
conhecida, não uma limitação de escopo — o exemplo
[`entradas/C11-fibonacci-recursivo.c`](../entradas/C11-fibonacci-recursivo.c) expõe esse caso
exatamente. A correção (salvar temporários vivos na pilha antes de um `jal` e recarregá-los
depois, ou reservar registradores "callee-saved" `s1`–`s11` para valores que atravessam
chamadas) fica como próximo passo, caso o projeto queira estender a fase de codegen.

---

## O problema do tamanho do frame: buffering

O prólogo de uma função (`addi sp, sp, -N`) precisa saber `N` — o tamanho do frame — mas esse
valor só é conhecido depois que **todas** as declarações locais da função foram processadas
(elas podem aparecer em qualquer ponto do corpo, inclusive dentro de blocos aninhados).

A solução foi **gerar o corpo da função num buffer separado** e só escrever o prólogo real
quando a função termina:

```cpp
// code_generator.h
inline std::ostringstream codegen_func_buffer;
inline bool               codegen_in_func = false;

// code_generator.cpp — codegen_emit()
void codegen_emit(const std::string& line)
{
    if (codegen_in_func)
        codegen_func_buffer << line << "\n";   // corpo vai para o buffer
    else if (codegen_main_out)
        *codegen_main_out << line << "\n";     // fora de função, vai direto
}
```

Fluxo em `parse_GlobalDecl()` (linha 136):
```cpp
if (codegen_enabled) codegen_func_begin(name_token.text); // limpa buffer, zera frame_bytes
parse_FuncDeclTail();                                      // corpo inteiro vai para o buffer
if (codegen_enabled) codegen_func_end();                   // agora frame_bytes é conhecido
```

`codegen_func_end()` calcula o frame (`frame_bytes + 8`, arredondado a 16 bytes), escreve o
prólogo diretamente na saída principal, despeja o conteúdo do buffer, e escreve o epílogo.

Cada parâmetro (`codegen_param`) e variável local (`codegen_alloc_local`) apenas incrementa
`codegen_frame_bytes` e registra seu offset relativo a `s0` em
`codegen_local_offsets` — a reserva de espaço acontece implicitamente pelo tamanho do frame,
calculado só no final.

---

## Vetores: endereço vs. valor

`ExprResult.reg` normalmente contém um **valor**. Para vetores, é preciso distinguir entre
"valor de um elemento" e "endereço base do vetor" em pontos diferentes da expressão.

Em `parse_PrimaryExpr()` (linha 953), ao reconhecer um identificador que é um array, o
gerador carrega o **endereço** (via `la`), não um valor:
```cpp
if (sym->is_array)
{
    std::string reg = codegen_alloc_ireg();
    codegen_emit("    la   " + reg + ", " + sym->name);
    return {sym->type, reg};   // reg é um ENDEREÇO, não um valor
}
```

Em `parse_PostfixTail()` (linha 1060), ao reconhecer `[índice]`, esse endereço é combinado
com o índice deslocado (`slli` + `add`) para formar o endereço do elemento, que então é lido
(`lw`/`flw`) para produzir o valor final da expressão:
```cpp
codegen_emit("    slli " + scaled + ", " + idx.reg + ", 2");
codegen_emit("    add  " + addr + ", " + addr + ", " + scaled);
codegen_lhs_addr_reg = addr;  // guardado para o caso de ser um LHS de atribuição
codegen_emit("    lw   " + res + ", 0(" + addr + ")");
```

### Atribuição a elemento de vetor

`array[i] = expr` é o caso mais delicado: o **endereço** do elemento precisa ser calculado a
partir do LHS *antes* de avaliar o RHS (que pode usar os mesmos registradores). A variável
global `codegen_lhs_addr_reg` guarda esse endereço; `parse_AssignTail()` (linha 572) o captura
no início, antes de recursivamente chamar `parse_Expr()` para o lado direito:

```cpp
std::string lhs_addr = codegen_lhs_addr_reg;
codegen_lhs_addr_reg = ""; // limpo para o RHS não interferir
ExprResult right = parse_Expr();
// ... depois, se lhs_addr não estiver vazio:
codegen_emit("    " + store_instr + "  " + right.reg + ", 0(" + lhs_addr + ")");
```

Para atribuição a uma variável simples (não-array), o caminho mais simples é usado:
`codegen_last_lhs_name` guarda o nome do identificador (não seu endereço), e
`codegen_store_var()` decide, olhando `codegen_local_offsets`, se é uma variável local
(`sw`/`fsw` relativo a `s0`) ou global (`la` + `sw`/`fsw`).

---

## Controle de fluxo: contadores de label

Como não há AST, não há como "voltar" e ajustar saltos depois. Cada estrutura de controle
gera seus próprios labels **no momento em que é reconhecida**, usando um contador global
(`codegen_label_count`) que garante nomes únicos mesmo com estruturas aninhadas:

```cpp
std::string codegen_new_label(const std::string& prefix)
{
    return "." + prefix + "_" + std::to_string(codegen_label_count++);
}
```

### if/else (`parse_IfStmt`, linha 342)

Dois labels (`else`, `end_if`) são reservados antes de qualquer código do corpo ser gerado,
permitindo que o `beqz` salte para um label que só será definido depois (assembly permite
referências a labels futuros).

### while / for (`parse_WhileStmt`, `parse_ForStmt`, linhas 374/404)

Mesmo princípio, com um detalhe em `for`: a condição de atualização (`i++`) fica **depois**
do corpo do laço no assembly (mais fácil de gerar sequencialmente), então a primeira
iteração precisa pular direto para o corpo, saltando a atualização:
```asm
j    .for_update_N_body      # primeira iteração pula a atualização
.for_update_N:
    ...                       # código de atualização (i++)
    j    .for_cond_N
.for_update_N_body:
    ...                       # corpo do laço
    j    .for_update_N        # ao final do corpo, sempre executa a atualização
```

### break / continue: pilha de labels

Cada `while`/`for` empilha um par `{continue_label, break_label}` em
`codegen_loop_stack` antes de gerar seu corpo, e desempilha ao final:

```cpp
struct LoopLabels { std::string continue_label; std::string break_label; };
inline std::vector<LoopLabels> codegen_loop_stack;
```

`parse_BreakStmt`/`parse_ContinueStmt` (linhas 514/528) apenas emitem `j` para o topo da
pilha. Como a pilha reflete o aninhamento real dos laços no momento do parsing, `break` e
`continue` sempre afetam o laço mais interno automaticamente — sem precisar rotular laços
explicitamente. Usar `break`/`continue` fora de qualquer laço (pilha vazia) gera um erro
semântico (`'break' fora de um laco`).

No `for`, o label de "continue" aponta para a **atualização** (`lbl_update`), não para a
condição — senão o incremento nunca seria executado.

---

## Chamadas de função

Argumentos são movidos para `a0`–`a7`/`fa0`–`fa7` em `parse_Args`/`parse_ArgsTail` (linha
1157), na ordem em que aparecem, via um contador `codegen_arg_index` resetado a cada chamada.
A chamada em si (`parse_PostfixTail`, ao encontrar `(`) usa `jal ra, nome_da_funcao`, e o
retorno é copiado de `a0`/`fa0` para um novo temporário, para poder ser combinado com o
restante da expressão.

Como a tabela de símbolos já registra a função **antes** de seu corpo ser parseado
(`parse_GlobalDecl`), chamadas recursivas são resolvidas normalmente pelo `semantic_lookup` —
não é necessário nenhum mecanismo especial de "forward declaration".

---

## Ativação condicionada a análise sem erros

A geração de código roda **durante** a mesma passagem da análise sintática/semântica (não há
segunda passagem), escrevendo em um `std::ostringstream` (`main.cpp`). Ao final, o arquivo
`.s` só é efetivamente escrito em disco se não houve nenhum erro sintático ou semântico:

```cpp
// main.cpp
if (config.emit_asm) {
    if (syn_ok && !semantic_error_found) {
        // grava asm_buffer no arquivo .s
    } else {
        std::cerr << ">>> Assembly nao gerado devido a erros na analise.\n";
    }
}
```

Isso evita gerar assembly de programas semanticamente inválidos, mesmo que boa parte do
código já tenha sido emitida corretamente até o ponto do erro.

---

## O que ficou fora de escopo (por decisão didática)

- **Otimizações**: nenhuma — cada subexpressão é avaliada e armazenada em um novo
  registrador, mesmo quando reaproveitar um valor já calculado seria possível
- **Spill de registradores**: se uma expressão excede 7 temporários simultâneos, o gerador
  lança uma exceção em vez de usar a pilha como memória auxiliar
- **Arrays locais (na pilha)**: `parse_VarDeclStmt` não aloca espaço de frame para arrays
  declarados dentro de funções — apenas arrays globais são suportados na geração de código
- **Verificação de assinatura de função**: argumentos de chamada não são checados contra os
  parâmetros declarados (mesma limitação já existente na análise semântica)

Ver [`doc/assembly-guide.md`](assembly-guide.md) para a referência completa de instruções.
