//
// Created by fc on 4/21/26.
//

#ifndef BOOTLOADER_SYNTACTIC_ANALYSER_H
#define BOOTLOADER_SYNTACTIC_ANALYSER_H

#include "lexical_analyser.h"
#include <vector>

struct syntax_tree {
    int a;
};

Token peek();
Token peek_next();
void advance();
void match(TokenType::Type type);

void parse_programa();
void parse_declaracao_global();

syntax_tree syntactic_analyser (std::vector<Token>&, bool);

void programa();

// peek retorna token atual
// advance consome o token atual e move para o prox
// match(expected_type) verifica se o token atual é o esperado, se for avança, se não
// dispara um erro sintatico

#endif //BOOTLOADER_SYNTACTIC_ANALYSER_H