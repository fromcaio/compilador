//
// Created by fc on 7/4/26.
//

#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include "semantic_analyser.h"
#include <string>
#include <sstream>
#include <ostream>
#include <unordered_map>

// --- RESULTADO DE EXPRESSÃO ---
// Propaga tipo semântico + registrador físico que contém o valor.
struct ExprResult {
    TokenType::Type type = TYPE_UNRESOLVED;
    std::string reg;  // "t0", "ft1", "" se sem valor (void/unused)
};

// --- ESTADO GLOBAL ---

inline bool               codegen_enabled     = false;
inline std::ostream*      codegen_main_out    = nullptr;
inline std::ostringstream codegen_func_buffer;
inline bool               codegen_in_func     = false;
inline int                codegen_label_count = 0;
inline int                codegen_frame_bytes = 0;   // bytes ocupados por params + locais
inline std::string        codegen_func_end_label;
inline std::string        codegen_current_func;
inline std::string        codegen_last_lhs_name;     // nome do LHS para AssignTail
inline std::string        codegen_last_decl_name;    // nome da variável sendo declarada
inline std::string        codegen_lhs_addr_reg;      // endereço de elemento de array no LHS
inline int                codegen_arg_index   = 0;   // contador de argumento em parse_Args

// Offsets de locais/params relativos a s0 (frame pointer), valores negativos
inline std::unordered_map<std::string, int> codegen_local_offsets;

// Pilha de labels de loop, para break/continue
struct LoopLabels {
    std::string continue_label;
    std::string break_label;
};
inline std::vector<LoopLabels> codegen_loop_stack;

// Pools de registradores temporários
inline bool codegen_ireg_used[7] = {};   // t0–t6
inline bool codegen_freg_used[7] = {};   // ft0–ft6

// --- API ---

std::string codegen_alloc_ireg();
std::string codegen_alloc_freg();
void        codegen_free_reg(const std::string& reg);

// Emite para o buffer da função (ou para main_out se fora de função)
void        codegen_emit(const std::string& line);
// Sempre emite para main_out (cabeçalhos de seção, labels de função)
void        codegen_emit_global(const std::string& line);
// Gera label único: ".prefix_N"
std::string codegen_new_label(const std::string& prefix);

// Declaração de variável global (seção .data)
void codegen_global_var(const std::string& name, TokenType::Type type,
                        bool is_array, int array_size = 0);

// Início e fim de função
void codegen_func_begin(const std::string& name);
void codegen_func_end();

// Parâmetros: store a{index} na posição local
void codegen_param(const std::string& name, TokenType::Type type, int index);

// Variáveis locais: reserva slot no frame (sem emit)
void codegen_alloc_local(const std::string& name, TokenType::Type type);

// Carga e store de variável (local ou global)
ExprResult codegen_load_var(const std::string& name, TokenType::Type type);
void       codegen_store_var(const std::string& name, const std::string& reg,
                             TokenType::Type type);

// Converte tipo para sufixo de instrução float ("s") ou inteiro ("")
inline bool codegen_is_float_type(TokenType::Type t)
{
    return t == TokenType::Type::TYPE_FLOAT;
}

#endif // CODE_GENERATOR_H
