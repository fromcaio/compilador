//
// Created by fc on 4/21/26.
//

#ifndef BOOTLOADER_SYNTACTIC_ANALYSER_H
#define BOOTLOADER_SYNTACTIC_ANALYSER_H

#include "semantic_analyser.h"
#include <vector>

// --- PONTO DE ENTRADA DO ANALISADOR SINTÁTICO ---
// Retorna true se a análise concluiu sem erros sintáticos.
bool syntactic_analyser(std::vector<Token>& tokens, bool verbose);

// --- FUNÇÕES DE UTILIDADE LÉXICA ---
Token peek();
Token peek_next();
void advance();
void match(TokenType::Type expected);
void synchronize();
bool is_data_type(TokenType::Type type);
bool is_assign_op(TokenType::Type type);

// ==========================================
// MÉTODOS DO RECURSIVE DESCENT PARSER (LL1)
// ==========================================

// --- ESTRUTURA GLOBAL ---
void parse_Program();
void parse_GlobalDecl();
void parse_VarDeclTail(TokenType::Type expected = TYPE_UNRESOLVED);
void parse_FuncDeclTail();

// --- PARÂMETROS E BLOCOS ---
void parse_Params();
void parse_Param();
void parse_ParamsTail();
void parse_Block(bool manage_scope = true);

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

// --- EXPRESSÕES (retornam o tipo inferido da subexpressão) ---
TokenType::Type parse_Expr();
TokenType::Type parse_AssignTail(TokenType::Type left_type);

TokenType::Type parse_LogicalOrExpr();
TokenType::Type parse_LogicalOrTail(TokenType::Type left_type);

TokenType::Type parse_LogicalAndExpr();
TokenType::Type parse_LogicalAndTail(TokenType::Type left_type);

TokenType::Type parse_EqExpr();
TokenType::Type parse_EqTail(TokenType::Type left_type);

TokenType::Type parse_RelExpr();
TokenType::Type parse_RelTail(TokenType::Type left_type);

TokenType::Type parse_AddExpr();
TokenType::Type parse_AddTail(TokenType::Type left_type);

TokenType::Type parse_MultExpr();
TokenType::Type parse_MultTail(TokenType::Type left_type);

TokenType::Type parse_UnaryExpr();
TokenType::Type parse_PrimaryExpr();
TokenType::Type parse_PostfixTail(TokenType::Type base_type);

// Argumentos de chamada — tipo ignorado (sem checagem de assinatura)
void parse_Args();
void parse_ArgsTail();

#endif //BOOTLOADER_SYNTACTIC_ANALYSER_H
