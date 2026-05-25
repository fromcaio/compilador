//
// Created by fc on 4/21/26.
//

#ifndef BOOTLOADER_SYNTACTIC_ANALYSER_H
#define BOOTLOADER_SYNTACTIC_ANALYSER_H

#include "lexical_analyser.h"
#include <vector>

// Estrutura provisória da árvore sintática (para não quebrar a main)
// Será implementada nas próximas etapas.
struct syntax_tree {
    int dummy;
};

// --- FUNÇÕES DE UTILIDADE LÉXICA ---
Token peek();
Token peek_next();
void advance();
void match(TokenType::Type expected);
void synchronize(); // Recuperação de modo pânico
bool is_data_type(TokenType::Type type);

// --- PONTO DE ENTRADA DO ANALISADOR SINTÁTICO ---
syntax_tree syntactic_analyser(std::vector<Token>& tokens, bool verbose);


// ==========================================
// MÉTODOS DO RECURSIVE DESCENT PARSER (LL1)
// ==========================================

// --- ESTRUTURA GLOBAL ---
void parse_Program();
void parse_GlobalDecl();
void parse_VarDeclTail();
void parse_FuncDeclTail();

// --- PARÂMETROS E BLOCOS ---
void parse_Params();
void parse_Param();
void parse_ParamsTail();
void parse_Block();

// --- STATEMENTS (COMANDOS) ---
void parse_Statements();
void parse_Statement();
void parse_VarDeclStmt();
void parse_IfStmt();
void parse_WhileStmt();
void parse_ForStmt();
void parse_ForInit();
void parse_ForUpdate();
void parse_ReturnStmt();
void parse_ReturnTail();
void parse_BreakStmt();
void parse_ContinueStmt();
void parse_ExprStmt();

// --- EXPRESSÕES (EXPRESSIONS) ---
void parse_Expr();
void parse_AssignTail();

void parse_LogicalOrExpr();
void parse_LogicalOrTail();

void parse_LogicalAndExpr();
void parse_LogicalAndTail();

void parse_EqExpr();
void parse_EqTail();

void parse_RelExpr();
void parse_RelTail();

void parse_AddExpr();
void parse_AddTail();

void parse_MultExpr();
void parse_MultTail();

void parse_UnaryExpr();

void parse_PrimaryExpr();
void parse_PostfixTail();

void parse_Args();
void parse_ArgsTail();

#endif //BOOTLOADER_SYNTACTIC_ANALYSER_H