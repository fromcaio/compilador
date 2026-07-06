# Guia do Assembly RISC-V Gerado

Este documento descreve a estrutura do assembly RISC-V 32 bits (RV32IMF) produzido pelo
compilador FCC e lista todas as instruções que o gerador de código pode emitir, com exemplos
extraídos de saídas reais.

## Por que RV32?

O compilador usa **RV32I + extensões M (multiplicação/divisão inteira) e F (ponto flutuante
de precisão simples)**. A escolha se justifica porque:

- `int` da linguagem mapeia diretamente para palavras de 32 bits
- `float` é precisão simples (32 bits) — coberto pela extensão F
- A convenção de chamada de 32 bits é mais simples de acompanhar didaticamente

Para montar e executar o código gerado em uma máquina 64 bits (ex: via WSL), usa-se o
compilador `riscv64-linux-gnu-gcc` com as flags `-march=rv32imf -mabi=ilp32f`, produzindo um
binário RV32 mesmo a partir de uma toolchain 64 bits.

---

## Estrutura Geral do Arquivo `.s`

Um arquivo gerado tem até três seções:

```asm
.data
globalVar: .word 0        # variáveis globais int/bool
values:    .space 40      # vetor global (10 elementos × 4 bytes)

.section .rodata
.fconst_0: .float 3.5     # constantes literais de ponto flutuante
.strconst_1: .string "ok" # literais de string

.text
.globl nome_da_funcao
nome_da_funcao:
    ...                    # corpo da função
```

- **`.data`**: variáveis globais (`int`, `float`, `bool`, vetores). Cada `codegen_global_var`
  emite seu próprio cabeçalho `.data` antes da declaração.
- **`.section .rodata`**: constantes imutáveis geradas ao encontrar literais numéricos de
  ponto flutuante ou literais de string dentro de expressões.
- **`.text`**: uma entrada por função, precedida por `.globl <nome>`.

---

## Convenção de Chamada (ABI RV32)

| Registrador(es) | Uso |
| :--- | :--- |
| `a0`–`a7` | Argumentos inteiros / retorno inteiro (`a0`) |
| `fa0`–`fa7` | Argumentos float / retorno float (`fa0`) |
| `ra` | Endereço de retorno (salvo/restaurado no prólogo/epílogo) |
| `sp` | Ponteiro de pilha |
| `s0` | Frame pointer (`fp`) — base para acessar locais e parâmetros |
| `t0`–`t6` | Temporários inteiros (pool de alocação do gerador) |
| `ft0`–`ft6` | Temporários float (pool de alocação do gerador) |

> **Limitação didática**: os registradores temporários (`t0`–`t6`, `ft0`–`ft6`) **não são
> salvos entre chamadas de função**. Isso é consistente com a convenção RISC-V (são
> "caller-saved"), mas o gerador não insere spill/reload automático — então uma expressão
> que precise manter um temporário vivo através de **duas chamadas de função** (como em
> `fib(n-1) + fib(n-2)`) pode produzir resultado incorreto. Ver
> [`doc/code-generation.md`](code-generation.md) para detalhes.

---

## Layout do Frame de Pilha

```
  s0  → [ ra salvo   ]  frame-4
        [ s0 salvo   ]  frame-8
        [ parâmetro 1]  frame-12
        [ parâmetro 2]  frame-16
        [ local 1    ]  frame-20
        ...
  sp  → (base do frame, sp = s0 - frame)
```

O tamanho do frame só é conhecido depois que todas as declarações locais da função são
processadas. Por isso, o corpo da função é gerado num buffer temporário
(`codegen_func_buffer`) e o prólogo real (com o tamanho correto) é emitido apenas quando a
função é fechada — ver `codegen_func_end()` em
[`code_generator.cpp`](../src/SourceFiles/code_generator.cpp).

Prólogo típico:
```asm
addi sp, sp, -32     # reserva o frame (alinhado a 16 bytes)
sw   ra, 28(sp)      # salva endereço de retorno
sw   s0, 24(sp)      # salva frame pointer do chamador
addi s0, sp, 32      # define novo frame pointer
```

Epílogo típico:
```asm
.func_end_0:
    lw   ra, 28(sp)
    lw   s0, 24(sp)
    addi sp, sp, 32
    ret
```

---

## Catálogo de Instruções Geradas

### Movimentação e constantes

| Instrução | Significado | Exemplo |
| :--- | :--- | :--- |
| `li rd, imm` | Carrega constante inteira imediata | `li   t0, 10` |
| `mv rd, rs` | Copia valor entre registradores inteiros | `mv   a0, t0` |
| `fmv.s rd, rs` | Copia valor entre registradores float | `fmv.s fa0, ft0` |
| `la rd, label` | Carrega o endereço de um símbolo (`.data`/`.rodata`) | `la   t0, globalVar` |

### Acesso à memória

| Instrução | Significado | Exemplo |
| :--- | :--- | :--- |
| `lw rd, off(rs)` | Carrega palavra (int/bool) de memória | `lw   t0, -12(s0)` |
| `sw rs, off(rd)` | Armazena palavra em memória | `sw   t0, -12(s0)` |
| `flw rd, off(rs)` | Carrega valor float de memória | `flw  ft0, 0(t0)` |
| `fsw rs, off(rd)` | Armazena valor float em memória | `fsw  ft0, -16(s0)` |

Usadas tanto para variáveis locais (offset relativo a `s0`) quanto para globais (endereço
carregado antes via `la`).

### Aritmética inteira

| Instrução | Significado | Exemplo |
| :--- | :--- | :--- |
| `add rd, rs1, rs2` | Soma | `add  t2, t0, t1` |
| `sub rd, rs1, rs2` | Subtração | `sub  t2, t0, t1` |
| `mul rd, rs1, rs2` | Multiplicação (extensão M) | `mul  t2, t0, t1` |
| `div rd, rs1, rs2` | Divisão (extensão M) | `div  t2, t0, t1` |
| `rem rd, rs1, rs2` | Resto da divisão / módulo (extensão M) | `rem  t2, t0, t1` |
| `neg rd, rs` | Negação (usado no operador unário `-`) | `neg  t1, t0` |
| `addi rd, rs, imm` | Soma com imediato (usado em `++`/`--`) | `addi t1, t0, 1` |

### Aritmética em ponto flutuante (extensão F)

| Instrução | Significado | Exemplo |
| :--- | :--- | :--- |
| `fadd.s rd, rs1, rs2` | Soma float | `fadd.s ft2, ft0, ft1` |
| `fsub.s rd, rs1, rs2` | Subtração float | `fsub.s ft2, ft0, ft1` |
| `fmul.s rd, rs1, rs2` | Multiplicação float | `fmul.s ft2, ft0, ft1` |
| `fdiv.s rd, rs1, rs2` | Divisão float | `fdiv.s ft2, ft0, ft1` |
| `fneg.s rd, rs` | Negação float (operador unário `-`) | `fneg.s ft1, ft0` |

### Comparações (produzem 0 ou 1, tipo `bool`)

| Instrução | Significado | Exemplo |
| :--- | :--- | :--- |
| `slt rd, rs1, rs2` | 1 se `rs1 < rs2`, senão 0. Base para `<`, `>`, `<=`, `>=` | `slt  t2, t0, t1` |
| `xori rd, rs, 1` | Inverte bit 0 (usado para `<=` e `>=`, a partir de `slt` invertido) | `xori t2, t2, 1` |
| `seqz rd, rs` | 1 se `rs == 0` (usado para `==` e `!`, após `sub`) | `seqz t2, t2` |
| `snez rd, rs` | 1 se `rs != 0` (usado para `!=`, após `sub`) | `snez t2, t2` |

Padrões de tradução de comparações:
```asm
# a < b
slt  t2, a, b

# a > b
slt  t2, b, a

# a <= b
slt  t2, b, a
xori t2, t2, 1

# a >= b
slt  t2, a, b
xori t2, t2, 1

# a == b
sub  t2, a, b
seqz t2, t2

# a != b
sub  t2, a, b
snez t2, t2
```

### Lógicos bit a bit (operam sobre valores booleanos 0/1)

| Instrução | Significado | Exemplo |
| :--- | :--- | :--- |
| `and rd, rs1, rs2` | Operador `&&` | `and  t2, t0, t1` |
| `or rd, rs1, rs2` | Operador `\|\|` | `or   t2, t0, t1` |

### Deslocamento (indexação de vetores)

| Instrução | Significado | Exemplo |
| :--- | :--- | :--- |
| `slli rd, rs, imm` | Desloca à esquerda (multiplica índice por 4, tamanho de uma palavra) | `slli t1, t0, 2` |

Acesso a `vetor[i]` sempre segue o padrão: `endereço_base + (i << 2)`.

### Controle de fluxo

| Instrução | Significado | Exemplo |
| :--- | :--- | :--- |
| `j label` | Salto incondicional | `j    .while_end_2` |
| `beqz rs, label` | Salta se `rs == 0` (usado para condições falsas) | `beqz t0, .else_1` |
| `jal ra, label` | Chamada de função (salva retorno em `ra`) | `jal  ra, sum` |
| `ret` | Retorno de função (`jr ra`) | `ret` |

Labels seguem o padrão `.<contexto>_<contador>`, gerados por `codegen_new_label()` para
garantir nomes únicos mesmo em estruturas aninhadas: `.else_N`, `.end_if_N`,
`.while_cond_N`, `.while_end_N`, `.for_cond_N`, `.for_update_N`, `.for_end_N`,
`.fconst_N`, `.strconst_N`, `.func_end_N`.

---

## Exemplo Completo Comentado

Fonte (`entradas/C04-condicional-if-else.c`):
```c
int maximo() {
    int a; int b; int max;
    a = 7; b = 12;
    if (a > b) { max = a; } else { max = b; }
    return max;
}
```

Assembly gerado (simplificado):
```asm
.text
.globl maximo
maximo:
    addi sp, sp, -32          # prólogo: reserva frame
    sw   ra, 28(sp)
    sw   s0, 24(sp)
    addi s0, sp, 32

    li   t0, 7                # a = 7
    sw   t0, -12(s0)
    li   t0, 12                # b = 12
    sw   t0, -16(s0)

    lw   t0, -12(s0)           # avalia (a > b)
    lw   t1, -16(s0)
    slt  t2, t1, t0
    beqz t2, .else_1           # se falso, pula para o else

    lw   t0, -12(s0)           # max = a
    sw   t0, -20(s0)
    j    .end_if_2
.else_1:
    lw   t0, -16(s0)           # max = b
    sw   t0, -20(s0)
.end_if_2:

    lw   t0, -20(s0)           # return max
    mv   a0, t0
    j    .func_end_0
.func_end_0:
    lw   ra, 28(sp)            # epílogo
    lw   s0, 24(sp)
    addi sp, sp, 32
    ret
```

Veja [`doc/code-generation.md`](code-generation.md) para como cada trecho é produzido pelo
parser.
