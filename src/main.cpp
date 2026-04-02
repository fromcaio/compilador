#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <set>
#include <fstream>
#include <nlohmann/json.hpp>

#define MAX_CHAR_LEN_UNFINISHED_STR_AND_COMMENTS 20

// --- CONFIGURAÇÕES GLOBAIS DA LINGUAGEM ---

const std::vector<std::string> arithmetic_operators = {
    "+", "-", "*", "/", "%", "++", "--", "+=", "-=", "*=", "/=", "%="
};
const std::vector<std::string> comparison_operators = {"==", "!=", "<", ">", "<=", ">="};
const std::vector<std::string> logical_operators = {"&&", "||", "!"};

const std::vector<std::string> token_separators = {
    " ", "\t", "\n", "(", ")", "{", "}", "[", "]", ";", ",", ":", "\"", "\'"
};
const std::vector<std::string> keywords = {
    "if", "else", "while", "for", "return", "int", "float", "string", "bool", "break", "continue", "complex", "void"
};

// Definições de Regex (PCRE compatível)
const std::string identifier_pattern = "^[a-zA-Z_][a-zA-Z0-9_]*$";
const std::string string_literal_pattern = R"(^"(\\.|[^"\\])*"$)";
const std::string number_literal_pattern = "^[0-9]+(\\.[0-9]+)?$";

// Definições de Regex dos Erros LEXICAL_ERROR_INVALID_CHAR, LEXICAL_ERROR_MALFORMED_NUMBER, LEXICAL_ERROR_UNCLOSED_STRING, LEXICAL_ERROR_UNCLOSED_COMMENT
const std::string lexical_error_malformed_number = R"(^[0-9]+(\.[0-9]+){2,}.*|^[0-9]+[a-zA-Z_]+.*)";
const std::string lexical_error_unclosed_string = R"(^"(\\.|[^"\\])*$)";

// Variável global que indica a presença de erros
bool lexical_errors = false;


// --- ESTRUTURAS DE DADOS ---

namespace TokenType {
    enum class Type {
        KEYWORD,
        IDENTIFIER,
        NUMBER_LITERAL,
        STRING_LITERAL,
        OPERATOR,
        SEPARATOR,
        ERROR
    };
}

struct Token {
    TokenType::Type type;
    std::string type_str; // Para especificação de erros
    std::string text;
    int line;
    int column;
};

// --- LÓGICA INTERNA (ENCAPSULADA) ---

namespace {
    // Constrói regex de keywords: ^(if|else|while|...)$
    std::string build_keyword_regex(const std::vector<std::string> &words) {
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
int classify_and_add_token(std::vector<size_t> &token_error_indices, std::vector<Token> &tokens,
                           const std::string &lexeme, int line, int col) {
    static const std::regex re_kw(build_keyword_regex(keywords));
    static const std::regex re_id(identifier_pattern);
    static const std::regex re_num(number_literal_pattern);
    static const std::regex re_str(string_literal_pattern);
    static const std::regex re_err_num(lexical_error_malformed_number);
    static const std::regex re_err_string(lexical_error_unclosed_string);

    if (std::regex_match(lexeme, re_kw)) tokens.push_back({TokenType::Type::KEYWORD, "KEYWORD", lexeme, line, col});
    else if (std::regex_match(lexeme, re_id))
        tokens.push_back({
            TokenType::Type::IDENTIFIER, "IDENTIFIER", lexeme, line, col
        });
    else if (std::regex_match(lexeme, re_num))
        tokens.push_back({
            TokenType::Type::NUMBER_LITERAL, "NUMBER_LITERAL", lexeme, line, col
        });
    else if (std::regex_match(lexeme, re_str))
        tokens.push_back({
            TokenType::Type::STRING_LITERAL, "STRING_LITERAL", lexeme, line, col
        });
    else {
        std::string tmp = "LEXICAL_ERROR_INVALID_CHAR";
        if (std::regex_match(lexeme, re_err_num)) tmp = "LEXICAL_ERROR_MALFORMED_NUMBER";
        else if (std::regex_match(lexeme, re_err_string)) tmp = "LEXICAL_ERROR_UNCLOSED_STRING";

        tokens.push_back({TokenType::Type::ERROR, tmp, lexeme, line, col});
        if (lexical_errors == false) lexical_errors = true;
        token_error_indices.push_back(tokens.size() - 1);
        return 0;
    }
    return 1;
}

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

// --- PONTO DE ENTRADA ---
// vamos receber como parametro de entrada um codigo fonte em C, flags --verbose e -o com o nome do arquivo de saida para fazer isso vamos

int main(int argc, char *argv[]) {
    std::string input_path;
    std::string output_path = "output.json"; // Valor padrão
    std::vector<size_t> token_error_indices;
    bool verbose = false;
    int block_commentary_line, block_commentary_column;

    // --- PROCESSAMENTO DE ARGUMENTOS ---
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "-o" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (input_path.empty()) {
            input_path = arg;
        }
    }

    if (input_path.empty()) {
        std::cerr << "Uso: ./fcc <arquivo.fcc> [-o saida.json] [--verbose]\n";
        return 1;
    }

    // --- LEITURA DO ARQUIVO ---
    std::ifstream file(input_path);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo: " << input_path << "\n";
        return 1;
    }

    std::vector<std::string> code_lines;
    std::string line;
    while (std::getline(file, line)) {
        code_lines.push_back(line + "\n");
    }
    file.close();

    // Pré-processamento de Operadores
    std::set<char> op_start_chars;
    std::vector<std::string> all_ops;
    all_ops.insert(all_ops.end(), arithmetic_operators.begin(), arithmetic_operators.end());
    all_ops.insert(all_ops.end(), comparison_operators.begin(), comparison_operators.end());
    all_ops.insert(all_ops.end(), logical_operators.begin(), logical_operators.end());

    std::vector<std::string> multi_char_ops;
    for (const auto &op: all_ops) {
        if (!op.empty()) op_start_chars.insert(op[0]);
        if (op.size() > 1) multi_char_ops.push_back(op);
    }

    std::vector<Token> tokens;
    bool in_block_comment = false;
    bool string_literal_started = false;
    int string_literal_line = 0, string_literal_column = 0;
    std::string current_lexeme;

    int line_num;
    // Loop Principal de Tokenização
    for (size_t i = 0; i < code_lines.size(); ++i) {
        std::string &line = code_lines[i];
        line_num = i + 1;

        if (!string_literal_started && !in_block_comment) current_lexeme.clear();

        for (size_t j = 0; j < line.size(); ++j) {
            char c = line[j];
            // --- INÍCIO DA LÓGICA DE COMENTÁRIOS ---
            if (in_block_comment) {
                if (c == '*' && j + 1 < line.size() && line[j + 1] == '/') {
                    in_block_comment = false;
                    j++;
                }
                if (i + 1 == code_lines.size() && j + 1 == line.size()) {
                    tokens.push_back({
                        TokenType::Type::ERROR, std::string{"LEXICAL_ERROR_UNCLOSED_COMMENT"}, current_lexeme.substr(0,MAX_CHAR_LEN_UNFINISHED_STR_AND_COMMENTS),
                        block_commentary_line, block_commentary_column
                    });
                    lexical_errors = true;
                    token_error_indices.push_back(tokens.size() - 1);
                }
                current_lexeme += c;
                continue;
            }

            if (c == '/' && j + 1 < line.size()) {
                if (line[j + 1] == '/') {
                    current_lexeme += c;
                    current_lexeme += line[j + 1];
                    break;
                }
                // Comentário de linha: ignora o resto da string
                if (line[j + 1] == '*') {
                    // Início de bloco
                    block_commentary_line = line_num;
                    block_commentary_column = static_cast<int>(j + 1);
                    in_block_comment = true;
                    current_lexeme += c;
                    current_lexeme += line[j + 1];
                    j++;
                    continue;
                }
            }

            // --- INÍCIO DA LÓGICA DE STRINGS ---
            // Caso o caractere seja aspas duplas e não tenhamos iniciado uma string
            if (c == '"' && !string_literal_started) {
                string_literal_started = true;
                string_literal_line = i + 1;
                string_literal_column = j + 1;
                current_lexeme += c;
                continue;
            }
            // Caso o caractere seja diferente de aspas duplas e já tenhamos iniciado a string
            if (string_literal_started && c != '"' && i < code_lines.size()) {
                // se estivermos na ultima linha e no ultimo caractere
                if (i + 1 == code_lines.size() && j + 1 == line.size()) {
                    current_lexeme += c;
                    tokens.push_back({TokenType::Type::ERROR, std::string{"LEXICAL_ERROR_UNCLOSED STRING"}, current_lexeme.substr(0,MAX_CHAR_LEN_UNFINISHED_STR_AND_COMMENTS), string_literal_line, string_literal_column});
                    lexical_errors = true;
                    token_error_indices.push_back(tokens.size() - 1);
                    continue;
                }
                current_lexeme += c;
                continue;
            }
            // Caso o caractere seja aspas duplas e já tenhamos iniciado uma string
            if (string_literal_started && c == '"') {
                current_lexeme += c;
                string_literal_started = false;
            }

            bool is_sep = std::find(token_separators.begin(), token_separators.end(), std::string(1, c)) !=
                          token_separators.end();
            bool is_op_trigger = op_start_chars.count(c);

            if (is_sep || is_op_trigger) {
                // Processa o que foi acumulado antes do separador
                if (!current_lexeme.empty()) {
                    classify_and_add_token(token_error_indices, tokens, current_lexeme, line_num,
                                           j - current_lexeme.size() + 1);
                    current_lexeme = "";
                }

                if (std::isspace(c)) continue;

                // Lógica de Lookahead para Operadores Compostos
                if (is_op_trigger && j + 1 < line.size()) {
                    std::string pot_op = std::string(1, c) + line[j + 1];
                    if (std::find(multi_char_ops.begin(), multi_char_ops.end(), pot_op) != multi_char_ops.end()) {
                        tokens.push_back({
                            TokenType::Type::OPERATOR, "OPERATOR", pot_op, line_num, static_cast<int>(j) + 1
                        });
                        continue;
                    }
                }

                // Separador ou Operador Simples
                std::string type = is_op_trigger ? "OPERATOR" : "SEPARATOR";
                TokenType::Type token_type = is_op_trigger ? TokenType::Type::OPERATOR : TokenType::Type::SEPARATOR;
                tokens.push_back({token_type, type, std::string(1, c), line_num, static_cast<int>(j) + 1});
            }
            current_lexeme += c;
        }
    }

    verbose ? verbose_output(tokens) : default_output(tokens, code_lines, token_error_indices);

    if (lexical_errors == false) {
        // --- SAÍDA JSON ---
        nlohmann::json j_tokens = nlohmann::json::array();
        for (const auto &t: tokens) {
            j_tokens.push_back({
                {"type", t.type},
                {"string_type", t.type_str},
                {"text", t.text},
                {"line", t.line},
                {"column", t.column}
            });
        }

        std::ofstream out_file(output_path);
        if (out_file.is_open()) {
            out_file << j_tokens.dump(4); // Indentação de 4 espaços
            out_file.close();
            std::cout << "Tokens salvos em: " << output_path << "\n";
        } else {
            std::cerr << "Erro ao salvar arquivo: " << output_path << "\n";
        }
    } else {
        std::cerr << "Foram encontrados erros lexicais. Nenhum arquivo de saída foi gerado.\n";
    }

    return 0;
}
