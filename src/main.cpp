#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <set>

// --- CONFIGURAÇÕES GLOBAIS DA LINGUAGEM ---

const std::vector<std::string> arithmetic_operators = {"+", "-", "*", "/", "%", "++", "--", "+=", "-=", "*=", "/=", "%="};
const std::vector<std::string> comparison_operators = {"==", "!=", "<", ">", "<=", ">="};
const std::vector<std::string> logical_operators    = {"&&", "||", "!"};

const std::vector<std::string> token_separators = {" ", "\t", "\n", "(", ")", "{", "}", "[", "]", ";", ",", ":", "\"", "\'"};
const std::vector<std::string> keywords         = {"if", "else", "while", "for", "return", "int", "float", "string", "bool", "break", "continue", "complex", "void"};

// Definições de Regex (PCRE compatível)
const std::string identifier_pattern     = "^[a-zA-Z_][a-zA-Z0-9_]*$";
const std::string string_literal_pattern = "^\"(\\\\.|[^\"\\\\])*\"$";
const std::string number_literal_pattern = "^[0-9]+(\\.[0-9]+)?$";

struct Token {
    std::string type;
    std::string text;
    int line;
    int column;
};

// --- LÓGICA INTERNA (ENCAPSULADA) ---

namespace {
    // Constrói regex de keywords: ^(if|else|while|...)$
    std::string build_keyword_regex(const std::vector<std::string>& words) {
        std::string pattern = "^(";
        for (size_t i = 0; i < words.size(); ++i) {
            pattern += words[i] + (i < words.size() - 1 ? "|" : "");
        }
        return pattern + ")$";
    }
}

/**
 * Classifica um lexema isolado usando Regex.
 * Prioridade: Keyword > Identifier > Number > String
 */
int classify_and_add_token(std::vector<Token>& tokens, const std::string& lexeme, int line, int col) {
    static const std::regex re_kw(build_keyword_regex(keywords));
    static const std::regex re_id(identifier_pattern);
    static const std::regex re_num(number_literal_pattern);
    static const std::regex re_str(string_literal_pattern);

    if (std::regex_match(lexeme, re_kw))       tokens.push_back({"KEYWORD", lexeme, line, col});
    else if (std::regex_match(lexeme, re_id))  tokens.push_back({"IDENTIFIER", lexeme, line, col});
    else if (std::regex_match(lexeme, re_num)) tokens.push_back({"NUMBER_LITERAL", lexeme, line, col});
    else if (std::regex_match(lexeme, re_str)) tokens.push_back({"STRING_LITERAL", lexeme, line, col});
    else {
        tokens.push_back({"ERROR", lexeme, line, col});
        return 0;
    }
    return 1;
}

// --- PONTO DE ENTRADA ---

int main() {
    std::vector<std::string> code_lines;
    std::string input_buffer;

    std::cout << "Lexer C++ | Insira o código (digite 'END' para processar):\n";
    while (std::getline(std::cin, input_buffer) && input_buffer != "END") {
        code_lines.push_back(input_buffer + "\n");
    }

    // Pré-processamento de Operadores
    std::set<char> op_start_chars;
    std::vector<std::string> all_ops;
    all_ops.insert(all_ops.end(), arithmetic_operators.begin(), arithmetic_operators.end());
    all_ops.insert(all_ops.end(), comparison_operators.begin(), comparison_operators.end());
    all_ops.insert(all_ops.end(), logical_operators.begin(), logical_operators.end());

    std::vector<std::string> multi_char_ops;
    for (const auto& op : all_ops) {
        if (!op.empty()) op_start_chars.insert(op[0]);
        if (op.size() > 1) multi_char_ops.push_back(op);
    }

    std::vector<Token> tokens;
    bool in_block_comment = false;

    // Loop Principal de Tokenização
    for (size_t i = 0; i < code_lines.size(); ++i) {
        std::string& line = code_lines[i];
        std::string current_lexeme = "";
        int line_num = i + 1;

        for (size_t j = 0; j < line.size(); ++j) {
            char c = line[j];
            // --- INÍCIO DA LÓGICA DE COMENTÁRIOS ---
            if (in_block_comment) {
                if (c == '*' && j + 1 < line.size() && line[j + 1] == '/') {
                    in_block_comment = false;
                    j++; 
                }
                continue;
            }

            if (c == '/' && j + 1 < line.size()) {
                if (line[j + 1] == '/') break; // Comentário de linha: ignora o resto da string
                if (line[j + 1] == '*') {      // Início de bloco
                    in_block_comment = true;
                    j++;
                    continue;
                }
            }
            
            bool is_sep = std::find(token_separators.begin(), token_separators.end(), std::string(1, c)) != token_separators.end();
            bool is_op_trigger = op_start_chars.count(c);

            if (is_sep || is_op_trigger) {
                // Processa o que foi acumulado antes do separador
                if (!current_lexeme.empty()) {
                    classify_and_add_token(tokens, current_lexeme, line_num, j - current_lexeme.size() + 1);
                    current_lexeme = "";
                }

                if (std::isspace(c)) continue;

                // Lógica de Lookahead para Operadores Compostos
                if (is_op_trigger && j + 1 < line.size()) {
                    std::string pot_op = std::string(1, c) + line[j+1];
                    if (std::find(multi_char_ops.begin(), multi_char_ops.end(), pot_op) != multi_char_ops.end()) {
                        tokens.push_back({"OPERATOR", pot_op, line_num, (int)j + 1});
                        j++; // Consome o caractere extra
                        continue;
                    }
                }

                // Separador ou Operador Simples
                std::string type = is_op_trigger ? "OPERATOR" : "SEPARATOR";
                tokens.push_back({type, std::string(1, c), line_num, (int)j + 1});
            } else {
                current_lexeme += c;
            }
        }
    }

    // Exibição dos Resultados
    std::cout << "\n--- TOKENS IDENTIFICADOS ---\n";
    for (const auto& t : tokens) {
        std::printf("[%s] '%s' (Linha: %d, Col: %d)\n", t.type.c_str(), t.text.c_str(), t.line, t.column);
    }

    return 0;
}