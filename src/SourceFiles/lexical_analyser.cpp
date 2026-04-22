//
// Created by fc on 4/20/26.
//
#include "../HeaderFiles/lexical_analyser.h"
#include "../HeaderFiles/lexical_analyser.h"
#include <algorithm>
#include <fstream>
#include <regex>
#include <set>
#include <nlohmann/json.hpp>

namespace {
    // Função auxiliar para busca na tabela estática
    const StaticTokenEntry* find_static_entry(const std::string& lexeme) {
        for (const auto& entry : reserved_map) {
            if (entry.lexeme == lexeme) return &entry;
        }
        return nullptr;
    }

    // Identifica caracteres que iniciam operadores (ex: '+', '-', '&')
    // Criado uma única vez para evitar uniões de vetores em runtime
    const std::set<char>& get_operator_triggers() {
        static std::set<char> triggers;
        if (triggers.empty()) {
            for (const auto& entry : reserved_map) {
                // Filtramos apenas o que é operador na nossa tabela
                if (entry.type >= TokenType::Type::OP_PLUS && entry.type <= TokenType::Type::OP_NOT) {
                    triggers.insert(entry.lexeme[0]);
                }
            }
        }
        return triggers;
    }
}

// --- PERSISTÊNCIA JSON ---

void save_tokens_to_json(const std::vector<Token>& tokens, const std::string& output_path) {
    nlohmann::json j_list = nlohmann::json::array();
    for (const auto &t : tokens) {
        j_list.push_back({
            {"id", static_cast<int>(t.type)},
            {"type", t.type_str},
            {"text", t.text},
            {"line", t.line},
            {"column", t.column}
        });
    }
    std::ofstream out_file(output_path);
    if (out_file.is_open()) {
        out_file << j_list.dump(4);
        std::cout << ">>> Analise lexica concluida. Tokens salvos em: " << output_path << "\n";
    }
}

void load_tokens_from_json(const std::string& input_path, std::vector<Token>& tokens) {
    std::ifstream file(input_path);
    if (!file.is_open()) return;

    nlohmann::json j_list;
    try {
        file >> j_list;
        for (const auto& item : j_list) {
            tokens.push_back({
                static_cast<TokenType::Type>(item.at("id").get<int>()),
                item.at("type").get<std::string>(),
                item.at("text").get<std::string>(),
                item.at("line").get<int>(),
                item.at("column").get<int>()
            });
        }
    } catch (...) { std::cerr << "Erro ao carregar JSON.\n"; }
}
std::string token_type_to_string(TokenType::Type type) {
    // Busca na nossa tabela estática primeiro
    for (const auto& entry : reserved_map) {
        if (entry.type == type) return entry.type_name;
    }

    // Casos que não estão no reserved_map (como IDENTIFIER, NUMBER_LITERAL, etc.)
    switch (type) {
        case TokenType::Type::IDENTIFIER:     return "IDENTIFIER";
        case TokenType::Type::NUMBER_LITERAL: return "NUMBER_LITERAL";
        case TokenType::Type::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::Type::ERROR:          return "ERROR";
        default:                              return "UNKNOWN_TOKEN";
    }
}

// --- CLASSIFICAÇÃO ---

int classify_and_add_token(std::vector<size_t> &token_error_indices, std::vector<Token> &tokens,
                           const std::string &lexeme, int line, int col) {

    // 1. Prioridade Máxima: Tabela Estática (Keywords e Operadores conhecidos)
    const StaticTokenEntry* entry = find_static_entry(lexeme);
    if (entry) {
        tokens.push_back({entry->type, entry->type_name, lexeme, line, col});
        return 1;
    }

    // 2. Prioridade Secundária: Padrões Dinâmicos (ID, Números, Strings)
    static const std::regex re_id(identifier_pattern);
    static const std::regex re_num(number_literal_pattern);
    static const std::regex re_str(string_literal_pattern);
    static const std::regex re_err_num(lexical_error_malformed_number);
    static const std::regex re_err_string(lexical_error_unclosed_string);

    if (std::regex_match(lexeme, re_id)) {
        tokens.push_back({TokenType::Type::IDENTIFIER, "IDENTIFIER", lexeme, line, col});
    }
    else if (std::regex_match(lexeme, re_num)) {
        tokens.push_back({TokenType::Type::NUMBER_LITERAL, "NUMBER_LITERAL", lexeme, line, col});
    }
    else if (std::regex_match(lexeme, re_str)) {
        tokens.push_back({TokenType::Type::STRING_LITERAL, "STRING_LITERAL", lexeme, line, col});
    }
    else {
        // 3. Tratamento de Erros
        std::string err_type = "LEXICAL_ERROR_INVALID_CHAR";
        if (std::regex_match(lexeme, re_err_num)) err_type = "LEXICAL_ERROR_MALFORMED_NUMBER";
        else if (std::regex_match(lexeme, re_err_string)) err_type = "LEXICAL_ERROR_UNCLOSED_STRING";

        tokens.push_back({TokenType::Type::ERROR, err_type, lexeme, line, col});
        lexical_errors = true;
        token_error_indices.push_back(tokens.size() - 1);
        return 0;
    }
    return 1;
}

// --- MOTOR LÉXICO ---

void run_lexical_analysis(const std::vector<std::string> &code_lines, std::vector<Token> &tokens,
    std::vector<size_t> &error_indices) {

    const auto& op_triggers = get_operator_triggers();
    bool in_block_comment = false;
    bool string_literal_started = false;
    int block_comment_line, block_comment_col;
    int str_line, str_col;
    std::string current_lexeme;

    for (size_t i = 0; i < code_lines.size(); ++i) {
        const std::string &line = code_lines[i];
        int line_num = static_cast<int>(i + 1);

        for (size_t j = 0; j < line.size(); ++j) {
            char c = line[j];

            if (in_block_comment) {
                if (c == '*' && j + 1 < line.size() && line[j + 1] == '/') {
                    in_block_comment = false;
                    j++;
                }
                continue;
            }

            if (c == '/' && j + 1 < line.size()) {
                if (line[j + 1] == '/') break;
                if (line[j + 1] == '*') {
                    in_block_comment = true;
                    block_comment_line = line_num; block_comment_col = j + 1;
                    j++; continue;
                }
            }

            // Identificação de separadores e gatilhos de operadores
            bool is_sep = std::find(token_separators.begin(), token_separators.end(), std::string(1, c)) != token_separators.end();
            bool is_op_trigger = op_triggers.count(c);

            if (is_sep || is_op_trigger) {
                if (!current_lexeme.empty()) {
                    classify_and_add_token(error_indices, tokens, current_lexeme, line_num, j - current_lexeme.size() + 1);
                    current_lexeme.clear();
                }

                if (std::isspace(c)) continue;

                // Lookahead para operadores multi-char
                if (is_op_trigger && j + 1 < line.size()) {
                    std::string pot_op = std::string(1, c) + line[j + 1];
                    const StaticTokenEntry* entry = find_static_entry(pot_op);
                    if (entry) {
                        tokens.push_back({entry->type, entry->type_name, pot_op, line_num, static_cast<int>(j + 1)});
                        j++; continue;
                    }
                }

                // Tenta classificar o caractere único via tabela
                const StaticTokenEntry* entry = find_static_entry(std::string(1, c));
                if (entry) {
                    tokens.push_back({entry->type, entry->type_name, entry->lexeme, line_num, static_cast<int>(j + 1)});
                }
            } else {
                current_lexeme += c;
            }
        }
    }

    // Verificações de fechamento (Comentários/Strings) omitidas para brevidade, mas devem ser mantidas
}

// ... verbose_output e default_output permanecem iguais
// --- SAÍDA FORMATADA ---
void verbose_output(const std::vector<Token> &tokens) {
    std::cout << "--- TOKENS IDENTIFICADOS (VERBOSE) ---\n";
    for (const auto &t: tokens) {
        std::printf("[%s] '%s' (Linha: %d, Col: %d)\n", t.type_str.c_str(), t.text.c_str(), t.line, t.column);
    }
}

void default_output(const std::vector<Token> &tokens, const std::vector<std::string> &code_lines,
                    const std::vector<size_t> &token_error_indices) {
    for (const size_t t: token_error_indices) {
        // vamos imprimir a linha sublinhada onde o erro ocorreu
        std::cerr << "Erro: " << tokens[t].type_str << " na linha " << tokens[t].line << ", coluna " << tokens[t].column
                << "\n";
        std::cerr << code_lines[tokens[t].line - 1]; // Imprime a linha do código
        std::cerr << std::string(tokens[t].column - 1, ' ') << "^\n"; // Aponta para a posição do erro
    }
}