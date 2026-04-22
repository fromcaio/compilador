//
// Created by fc on 4/21/26.
//
#include "../HeaderFiles/syntactic_analyser.h"
#include <iostream>

static size_t current_token_index;
static std::vector<Token> global_tokens; // Cópia local ou referência
static bool syntax_error_found;
static syntax_tree s_tree;
static bool  syntactic_analyser_verbosity;

Token peek() {
    if (current_token_index < global_tokens.size()) {
        return global_tokens[current_token_index];
    }
    // Retorna um token de fim de arquivo (EOF) se chegar ao fim
    return {TokenType::Type::ERROR, "EOF", "", -1, -1};
}

Token peek_next() {
    if (current_token_index + 1 < global_tokens.size()) {
        return global_tokens[current_token_index + 1];
    }
    return {TokenType::Type::ERROR, "EOF", "", -1, -1};
}

void advance() {
    if (current_token_index < global_tokens.size()) {
        current_token_index++;
    }
}

void synchronize() {
    // Enquanto o token atual NÃO for um ponto e vírgula
    // E NÃO for o fim do arquivo (ERROR + EOF)
    while (true) {
        Token t = peek();

        // Se chegamos no fim do arquivo, paramos a sincronização
        if (t.type == TokenType::Type::ERROR && t.type_str == "EOF") {
            break;
        }

        // Se achamos o ponto e vírgula, consumimos ele e paramos (sincronizado!)
        if (t.type == TokenType::Type::SEMICOLON) {
            advance();
            break;
        }

        // Caso contrário, ignoramos o token problemático e continuamos procurando
        advance();
    }
}

void match(TokenType::Type expected) {
    if (peek().type == expected) {
        advance();
    } else {
        std::cerr << "Erro Sintatico: Esperado [" << token_type_to_string(expected)
                  << "] mas encontrado [" << token_type_to_string(peek().type)
                  << "] ('" << peek().text << "') na linha " << peek().line
                  << ", coluna " << peek().column << std::endl;

        syntax_error_found = true;
        synchronize();
    }
}

bool is_data_type(TokenType::Type type) {
    return (type == TokenType::Type::TYPE_INT    ||
            type == TokenType::Type::TYPE_FLOAT  ||
            type == TokenType::Type::TYPE_BOOL   ||
            type == TokenType::Type::TYPE_STRING ||
            type == TokenType::Type::TYPE_VOID   ||
            type == TokenType::Type::TYPE_COMPLEX);
}

// Esta é a função inicial chamada por syntactic_analyser()
void parse_programa() {
    // Enquanto não chegarmos ao fim do arquivo (ERROR + EOF)
    while (true) {
        Token t = peek();

        // Verifica se é o fim do arquivo através da sua lógica de type_str
        if (t.type == TokenType::Type::ERROR && t.type_str == "EOF") {
            break;
        }

        // No escopo global do FCC, tudo deve começar com um TIPO (int, float, void...)
        if (is_data_type(t.type)) {
            parse_declaracao_global();
        } else {
            // Se encontrar algo que não começa com tipo no escopo global, é erro.
            std::cerr << "Erro Sintatico: Esperado um tipo no escopo global, mas encontrado ["
                      << t.type_str << "] na linha " << t.line << std::endl;
            syntax_error_found = true;

            // Sincroniza para tentar achar a próxima declaração válida
            synchronize();
        }
    }
}

void parse_declaracao_global() {
    advance();
    return;
}

syntax_tree syntactic_analyser(std::vector<Token>& tokens, bool verbose) {
    // 1. Inicializa/Reseta o estado interno
    global_tokens = tokens;
    current_token_index = 0;
    syntax_error_found = false;
    syntactic_analyser_verbosity = verbose;

    // 2. Chama a regra inicial da gramática
    parse_programa();

    // 3. Retorna o resultado final (a árvore construída)
    return s_tree;
}
