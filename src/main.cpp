#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <set>
#include <fstream>
#include <nlohmann/json.hpp>

// --- CONFIGURAÇÕES GLOBAIS DA LINGUAGEM ---

const std::vector<std::string> arithmetic_operators = {"+", "-", "*", "/", "%", "++", "--", "+=", "-=", "*=", "/=", "%="};
const std::vector<std::string> comparison_operators = {"==", "!=", "<", ">", "<=", ">="};
const std::vector<std::string> logical_operators    = {"&&", "||", "!"};

const std::vector<std::string> token_separators = {" ", "\t", "\n", "(", ")", "{", "}", "[", "]", ";", ",", ":", "\"", "\'"};
const std::vector<std::string> keywords         = {"if", "else", "while", "for", "return", "int", "float", "string", "bool", "break", "continue", "complex", "void"};

// Definições de Regex (PCRE compatível)
const std::string identifier_pattern     = "^[a-zA-Z_][a-zA-Z0-9_]*$";
const std::string string_literal_pattern = R"(^"(\\.|[^"\\])*"$)";
const std::string number_literal_pattern = "^[0-9]+(\\.[0-9]+)?$";

// Variável global para controle de erros lexicais
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
    std::string type_str; // Para saída legível
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

    if (std::regex_match(lexeme, re_kw))       tokens.push_back({TokenType::Type::KEYWORD, "KEYWORD", lexeme, line, col});
    else if (std::regex_match(lexeme, re_id))  tokens.push_back({TokenType::Type::IDENTIFIER, "IDENTIFIER", lexeme, line, col});
    else if (std::regex_match(lexeme, re_num)) tokens.push_back({TokenType::Type::NUMBER_LITERAL, "NUMBER_LITERAL", lexeme, line, col});
    else if (std::regex_match(lexeme, re_str)) tokens.push_back({TokenType::Type::STRING_LITERAL, "STRING_LITERAL", lexeme, line, col});
    else {
        tokens.push_back({TokenType::Type::ERROR, "ERROR", lexeme, line, col});
        if (lexical_errors == false) lexical_errors = true;
        return 0;
    }
    return 1;
}

// --- SAÍDA FORMATADA ---
void verbose_output(const std::vector<Token>& tokens) {
    std::cout << "--- TOKENS IDENTIFICADOS (VERBOSE) ---\n";
    for (const auto& t : tokens) {
        std::printf("[%s] '%s' (Linha: %d, Col: %d)\n", t.type_str.c_str(), t.text.c_str(), t.line, t.column);
    }
}

void default_output(const std::vector<Token>& tokens, const std::vector<std::string>& code_lines) {
    for (const auto& t : tokens) {
        // vamos imprimir a linha sublinhada onde o erro ocorreu
        if (t.type == TokenType::Type::ERROR) {
            std::cerr << "Erro: " << "na linha " << t.line << ", coluna " << t.column << "\n";
            std::cerr << code_lines[t.line - 1]; // Imprime a linha do código
            std::cerr << std::string(t.column - 1, ' ') << "^\n"; // Aponta para a posição do erro
        }
    }
}

// --- PONTO DE ENTRADA ---
// vamos receber como parametro de entrada um codigo fonte em C, flags --verbose e -o com o nome do arquivo de saida para fazer isso vamos

int main(int argc, char* argv[]) {
    std::string input_path;
    std::string output_path = "output.json"; // Valor padrão
    bool verbose = false;

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
                        tokens.push_back({TokenType::Type::OPERATOR, "OPERATOR", pot_op, line_num, (int)j + 1});
                        j++; // Consome o caractere extra
                        continue;
                    }
                }

                // Separador ou Operador Simples
                std::string type = is_op_trigger ? "OPERATOR" : "SEPARATOR";
                TokenType::Type token_type = is_op_trigger ? TokenType::Type::OPERATOR : TokenType::Type::SEPARATOR;
                tokens.push_back({token_type, type, std::string(1, c), line_num, (int)j + 1});
            } else {
                current_lexeme += c;
            }
        }
    }

    verbose ? verbose_output(tokens) : default_output(tokens, code_lines);

    if (lexical_errors == false) {
        // --- SAÍDA JSON ---
        nlohmann::json j_tokens = nlohmann::json::array();
        for (const auto& t : tokens) {
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