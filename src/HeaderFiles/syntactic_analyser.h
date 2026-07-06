//
// Created by fc on 4/21/26.
//

#ifndef BOOTLOADER_SYNTACTIC_ANALYSER_H
#define BOOTLOADER_SYNTACTIC_ANALYSER_H

#include "code_generator.h"
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

// --- EXPRESSÕES (retornam ExprResult: tipo semântico + registrador) ---
ExprResult parse_Expr();
ExprResult parse_AssignTail(ExprResult left);

ExprResult parse_LogicalOrExpr();
ExprResult parse_LogicalOrTail(ExprResult left);

ExprResult parse_LogicalAndExpr();
ExprResult parse_LogicalAndTail(ExprResult left);

ExprResult parse_EqExpr();
ExprResult parse_EqTail(ExprResult left);

ExprResult parse_RelExpr();
ExprResult parse_RelTail(ExprResult left);

ExprResult parse_AddExpr();
ExprResult parse_AddTail(ExprResult left);

ExprResult parse_MultExpr();
ExprResult parse_MultTail(ExprResult left);

ExprResult parse_UnaryExpr();
ExprResult parse_PrimaryExpr();
ExprResult parse_PostfixTail(ExprResult base);

// Argumentos de chamada — tipo ignorado (sem checagem de assinatura)
void parse_Args();
void parse_ArgsTail();

#endif //BOOTLOADER_SYNTACTIC_ANALYSER_H
