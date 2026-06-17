//
// Created by fc on 6/16/26.
//
#include "../HeaderFiles/semantic_analyser.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

// --- UTILITÁRIOS ---

bool is_unresolved(TokenType::Type t) {
    return t == TYPE_UNRESOLVED;
}

std::string type_to_string(TokenType::Type t) {
    switch (t) {
        case TokenType::Type::TYPE_INT:     return "int";
        case TokenType::Type::TYPE_FLOAT:   return "float";
        case TokenType::Type::TYPE_STRING:  return "string";
        case TokenType::Type::TYPE_BOOL:    return "bool";
        case TokenType::Type::TYPE_VOID:    return "void";
        case TokenType::Type::TYPE_COMPLEX: return "complex";
        default:                             return "?";
    }
}

// --- GERENCIAMENTO DE ESCOPO ---

void semantic_enter_scope() {
    current_scope++;
}

void semantic_exit_scope() {
    // Desativa todos os símbolos do escopo atual (mantidos na tabela para impressão)
    for (auto& sym : symbol_table) {
        if (sym.scope_level == current_scope && sym.active) {
            sym.active = false;
        }
    }
    current_scope--;
}

// --- DECLARAÇÃO E BUSCA ---

bool semantic_declare(const std::string& name, TokenType::Type type,
                      SymbolKind kind, int line, bool is_array) {
    // Verifica redeclaração no mesmo escopo (apenas símbolos ativos)
    for (const auto& sym : symbol_table) {
        if (sym.name == name && sym.scope_level == current_scope && sym.active) {
            semantic_report_error("Redeclaracao de '" + name + "' no mesmo escopo", line);
            return false;
        }
    }
    symbol_table.push_back({name, type, kind, current_scope, line, is_array, true});
    return true;
}

Symbol* semantic_lookup(const std::string& name) {
    // Busca do escopo mais interno para o mais externo (innermost wins)
    for (int i = static_cast<int>(symbol_table.size()) - 1; i >= 0; --i) {
        Symbol& sym = symbol_table[i];
        if (sym.name == name && sym.active && sym.scope_level <= current_scope) {
            return &sym;
        }
    }
    return nullptr;
}

void semantic_report_error(const std::string& msg, int line) {
    std::cerr << "Erro Semantico (linha " << line << "): " << msg << "\n";
    semantic_error_found = true;
}

// --- IMPRESSÃO DA TABELA ---

void print_symbol_table() {
    std::cout << "\n=== TABELA DE SIMBOLOS ===\n";

    if (symbol_table.empty()) {
        std::cout << "(tabela vazia)\n";
        return;
    }

    std::cout << std::left
              << std::setw(18) << "Nome"
              << std::setw(10) << "Tipo"
              << std::setw(8)  << "Escopo"
              << std::setw(7)  << "Linha"
              << "Categoria\n";
    std::cout << std::string(53, '-') << "\n";

    for (const auto& sym : symbol_table) {
        std::string type_str = type_to_string(sym.type);
        if (sym.is_array) type_str += "[]";
        std::string kind_str = (sym.kind == SymbolKind::FUNCTION) ? "FUNCAO" : "VARIAVEL";

        std::cout << std::left
                  << std::setw(18) << sym.name
                  << std::setw(10) << type_str
                  << std::setw(8)  << sym.scope_level
                  << std::setw(7)  << sym.declared_line
                  << kind_str << "\n";
    }
}
