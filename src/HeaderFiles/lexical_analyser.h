//
// Created by fc on 4/20/26.
//

#ifndef BOOTLOADER_LEXICALANALYSER_H
#define BOOTLOADER_LEXICALANALYSER_H

#include <string>
#include <vector>
#include <iostream>

#define MAX_CHAR_LEN_UNFINISHED_STR_AND_COMMENTS 20

// --- CONFIGURAÇÕES GLOBAIS DA LINGUAGEM ---
namespace TokenType {
    enum class Type {
        // Tipos de Dados
        TYPE_INT, TYPE_FLOAT, TYPE_STRING, TYPE_BOOL, TYPE_VOID, TYPE_COMPLEX,
        // Controle de Fluxo
        KW_IF, KW_ELSE, KW_WHILE, KW_FOR, KW_RETURN, KW_CONTINUE, KW_BREAK, KW_MAIN,
        // Identificadores e Literais
        IDENTIFIER, NUMBER_LITERAL, STRING_LITERAL,
        // Operadores Aritméticos
        OP_PLUS, OP_MINUS, OP_MULT, OP_DIV, OP_MOD,
        OP_INC, OP_DEC, // ++ e --
        OP_PLUS_ASSIGN, OP_MINUS_ASSIGN, OP_MULT_ASSIGN, OP_DIV_ASSIGN, OP_MOD_ASSIGN,
        // Operadores Relacionais e Lógicos
        OP_ASSIGN, OP_EQ, OP_NE, OP_LT, OP_GT, OP_LE, OP_GE,
        OP_AND, OP_OR, OP_NOT,
        // Separadores e Delimitadores
        L_PAREN, R_PAREN, L_BRACE, R_BRACE, L_BRACKET, R_BRACKET,
        SEMICOLON, COMMA, COLON,
        ERROR
    };
}

struct StaticTokenEntry {
    std::string lexeme;
    TokenType::Type type;
    std::string type_name;
};

inline const std::vector<StaticTokenEntry> reserved_map = {
    // Palavras-chave (Keywords)
    {"int",      TokenType::Type::TYPE_INT,       "TYPE_INT"},
    {"float",    TokenType::Type::TYPE_FLOAT,     "TYPE_FLOAT"},
    {"string",   TokenType::Type::TYPE_STRING,    "TYPE_STRING"},
    {"bool",     TokenType::Type::TYPE_BOOL,      "TYPE_BOOL"},
    {"void",     TokenType::Type::TYPE_VOID,      "TYPE_VOID"},
    {"complex",  TokenType::Type::TYPE_COMPLEX,   "TYPE_COMPLEX"},
    {"if",       TokenType::Type::KW_IF,          "KW_IF"},
    {"else",     TokenType::Type::KW_ELSE,        "KW_ELSE"},
    {"while",    TokenType::Type::KW_WHILE,       "KW_WHILE"},
    {"for",      TokenType::Type::KW_FOR,         "KW_FOR"},
    {"return",   TokenType::Type::KW_RETURN,      "KW_RETURN"},
    {"break",    TokenType::Type::KW_BREAK,       "KW_BREAK"},
    {"continue", TokenType::Type::KW_CONTINUE,    "KW_CONTINUE"},
    {"main",     TokenType::Type::KW_MAIN,        "KW_MAIN"},

    // Operadores Aritméticos e Atribuição
    {"=",        TokenType::Type::OP_ASSIGN,       "OP_ASSIGN"},
    {"+",        TokenType::Type::OP_PLUS,         "OP_PLUS"},
    {"-",        TokenType::Type::OP_MINUS,        "OP_MINUS"},
    {"*",        TokenType::Type::OP_MULT,         "OP_MULT"},
    {"/",        TokenType::Type::OP_DIV,          "OP_DIV"},
    {"%",        TokenType::Type::OP_MOD,          "OP_MOD"},
    {"++",       TokenType::Type::OP_INC,          "OP_INC"},
    {"--",       TokenType::Type::OP_DEC,          "OP_DEC"},
    {"+=",       TokenType::Type::OP_PLUS_ASSIGN,  "OP_PLUS_ASSIGN"},
    {"-=",       TokenType::Type::OP_MINUS_ASSIGN, "OP_MINUS_ASSIGN"},
    {"*=",       TokenType::Type::OP_MULT_ASSIGN,  "OP_MULT_ASSIGN"},
    {"/=",       TokenType::Type::OP_DIV_ASSIGN,   "OP_DIV_ASSIGN"},
    {"%=",       TokenType::Type::OP_MOD_ASSIGN,   "OP_MOD_ASSIGN"},

    // Operadores Lógicos e Comparação
    {"==",       TokenType::Type::OP_EQ,           "OP_EQ"},
    {"!=",       TokenType::Type::OP_NE,           "OP_NE"},
    {"<",        TokenType::Type::OP_LT,           "OP_LT"},
    {">",        TokenType::Type::OP_GT,           "OP_GT"},
    {"<=",       TokenType::Type::OP_LE,           "OP_LE"},
    {">=",       TokenType::Type::OP_GE,           "OP_GE"},
    {"&&",       TokenType::Type::OP_AND,          "OP_AND"},
    {"||",       TokenType::Type::OP_OR,           "OP_OR"},
    {"!",        TokenType::Type::OP_NOT,          "OP_NOT"},

    // Separadores e Pontuação
    {"(",        TokenType::Type::L_PAREN,         "L_PAREN"},
    {")",        TokenType::Type::R_PAREN,         "R_PAREN"},
    {"{",        TokenType::Type::L_BRACE,         "L_BRACE"},
    {"}",        TokenType::Type::R_BRACE,         "R_BRACE"},
    {"[",        TokenType::Type::L_BRACKET,       "L_BRACKET"},
    {"]",        TokenType::Type::R_BRACKET,       "R_BRACKET"},
    {";",        TokenType::Type::SEMICOLON,       "SEMICOLON"},
    {",",        TokenType::Type::COMMA,           "COMMA"},
    {":",        TokenType::Type::COLON,           "COLON"}
};

inline const std::vector<std::string> arithmetic_operators = {
    "+", "-", "*", "/", "%", "++", "--", "+=", "-=", "*=", "/=", "%="
};
inline const std::vector<std::string> comparison_operators = {"==", "!=", "<", ">", "<=", ">="};
inline const std::vector<std::string> logical_operators = {"&&", "||", "!"};

inline const std::vector<std::string> token_separators = {
    " ", "\t", "\n", "(", ")", "{", "}", "[", "]", ";", ",", ":", "\"", "\'"
};
inline const std::vector<std::string> keywords = {
    "if", "else", "while", "for", "return", "int", "float", "string", "bool", "break", "continue", "void"
};

// Definições de Regex (PCRE compatível)
inline const std::string identifier_pattern = "^[a-zA-Z_][a-zA-Z0-9_]*$";
inline const std::string string_literal_pattern = R"(^"(\\.|[^"\\])*"$)";
inline const std::string number_literal_pattern = "^[0-9]+(\\.[0-9]+)?$";

// Definições de Regex dos Erros LEXICAL_ERROR_INVALID_CHAR, LEXICAL_ERROR_MALFORMED_NUMBER, LEXICAL_ERROR_UNCLOSED_STRING, LEXICAL_ERROR_UNCLOSED_COMMENT
inline const std::string lexical_error_malformed_number = R"(^[0-9]+(\.[0-9]+){2,}.*|^[0-9]+[a-zA-Z_]+.*)";
inline const std::string lexical_error_unclosed_string = R"(^"(\\.|[^"\\])*$)";

// Variável global que indica a presença de erros
inline bool lexical_errors = false;


// --- ESTRUTURAS DE DADOS ---

struct Token {
    TokenType::Type type;
    std::string type_str; // Para especificação de erros
    std::string text;
    int line;
    int column;
};

void save_tokens_to_json(const std::vector<Token>&, const std::string& );

void load_tokens_from_json(const std::string&, std::vector<Token>& );

int classify_and_add_token(std::vector<size_t> &, std::vector<Token> &,
                           const std::string &, int, int);

std::string token_type_to_string(TokenType::Type);

// Função principal que executa a tokenização completa
void run_lexical_analysis(const std::vector<std::string>& code_lines,
                          std::vector<Token>& tokens,
                          std::vector<size_t>& error_indices);

// --- SAÍDA FORMATADA ---
void verbose_output(const std::vector<Token> &);

void default_output(const std::vector<Token> &, const std::vector<std::string> &,
                    const std::vector<size_t> &);


#endif //BOOTLOADER_LEXICALANALYSER_H
