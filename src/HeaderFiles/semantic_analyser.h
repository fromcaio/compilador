//
// Created by fc on 6/16/26.
//

#ifndef SEMANTIC_ANALYSER_H
#define SEMANTIC_ANALYSER_H

#include "lexical_analyser.h"
#include <vector>
#include <string>

// --- TABELA DE SÍMBOLOS ---

enum class SymbolKind { VARIABLE, FUNCTION };

struct Symbol {
    std::string name;
    TokenType::Type type;    // tipo base declarado
    SymbolKind kind;
    int scope_level;         // 0 = global
    int declared_line;
    bool is_array;
    bool active;             // false após o escopo ser encerrado
};

// Sentinela para tipo não resolvível (funções desconhecidas, erros de lookup, etc.)
constexpr TokenType::Type TYPE_UNRESOLVED = TokenType::Type::EOF_TOKEN;

// --- ESTADO GLOBAL ---

inline std::vector<Symbol> symbol_table;
inline int current_scope = 0;
inline bool semantic_error_found = false;
inline TokenType::Type current_function_return_type = TokenType::Type::TYPE_VOID;

// --- UTILITÁRIOS ---

bool is_unresolved(TokenType::Type t);
std::string type_to_string(TokenType::Type t);

// --- API PÚBLICA ---

void semantic_enter_scope();
void semantic_exit_scope();
bool semantic_declare(const std::string& name, TokenType::Type type,
                      SymbolKind kind, int line, bool is_array = false);
Symbol* semantic_lookup(const std::string& name);
void semantic_report_error(const std::string& msg, int line);
void print_symbol_table();

#endif //SEMANTIC_ANALYSER_H
