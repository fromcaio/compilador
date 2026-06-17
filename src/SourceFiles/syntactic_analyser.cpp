//
// Created by fc on 4/21/26.
//
#include "../HeaderFiles/syntactic_analyser.h"
#include <iostream>

static size_t current_token_index;
static std::vector<Token> global_tokens;
static bool syntax_error_found;
static bool syntactic_analyser_verbosity;

// ==========================================
// FUNÇÕES UTILITÁRIAS LÉXICAS
// ==========================================

Token peek()
{
    if (current_token_index < global_tokens.size())
    {
        return global_tokens[current_token_index];
    }
    return {TokenType::Type::EOF_TOKEN, "EOF_TOKEN", "EOF", -1, -1};
}

Token peek_next()
{
    if (current_token_index + 1 < global_tokens.size())
    {
        return global_tokens[current_token_index + 1];
    }
    return {TokenType::Type::EOF_TOKEN, "EOF_TOKEN", "EOF", -1, -1};
}

void advance()
{
    if (current_token_index < global_tokens.size() && peek().type != TokenType::Type::EOF_TOKEN)
    {
        current_token_index++;
    }
}

void synchronize()
{
    while (true)
    {
        Token t = peek();

        if (t.type == TokenType::Type::EOF_TOKEN)
            break;
        if (t.type == TokenType::Type::SEMICOLON)
            break;

        if (t.type == TokenType::Type::R_BRACE ||
            t.type == TokenType::Type::KW_IF ||
            t.type == TokenType::Type::KW_WHILE ||
            t.type == TokenType::Type::KW_FOR ||
            t.type == TokenType::Type::KW_RETURN ||
            t.type == TokenType::Type::KW_BREAK ||
            t.type == TokenType::Type::KW_CONTINUE ||
            is_data_type(t.type))
        {
            break;
        }

        advance();
    }
}

void match(TokenType::Type expected)
{
    if (peek().type == expected)
    {
        if (syntactic_analyser_verbosity)
        {
            std::cout << "    [MATCH] " << token_type_to_string(expected) << " ('" << peek().text << "')\n";
        }
        advance();
    }
    else
    {
        std::cerr << "Erro Sintatico: Esperado [" << token_type_to_string(expected)
                  << "] mas encontrado [" << token_type_to_string(peek().type)
                  << "] ('" << peek().text << "') na linha " << peek().line
                  << ", coluna " << peek().column << std::endl;
        syntax_error_found = true;
        synchronize();
    }
}

bool is_data_type(TokenType::Type type)
{
    return (type == TokenType::Type::TYPE_INT ||
            type == TokenType::Type::TYPE_FLOAT ||
            type == TokenType::Type::TYPE_BOOL ||
            type == TokenType::Type::TYPE_STRING ||
            type == TokenType::Type::TYPE_VOID ||
            type == TokenType::Type::TYPE_COMPLEX);
}

bool is_assign_op(TokenType::Type type)
{
    return (type == TokenType::Type::OP_ASSIGN || type == TokenType::Type::OP_PLUS_ASSIGN ||
            type == TokenType::Type::OP_MINUS_ASSIGN || type == TokenType::Type::OP_MULT_ASSIGN ||
            type == TokenType::Type::OP_DIV_ASSIGN || type == TokenType::Type::OP_MOD_ASSIGN);
}

// ==========================================
// ESTRUTURA GLOBAL
// ==========================================

void parse_Program()
{
    if (syntactic_analyser_verbosity)
        std::cout << "> parse_Program\n";
    while (true)
    {
        Token t = peek();
        if (t.type == TokenType::Type::EOF_TOKEN)
            break;

        if (is_data_type(t.type))
        {
            parse_GlobalDecl();
        }
        else
        {
            std::cerr << "Erro Sintatico: Esperado um tipo no escopo global, mas encontrado ["
                      << token_type_to_string(t.type) << "] ('" << t.text << "') na linha " << t.line << std::endl;
            syntax_error_found = true;
            advance();
            synchronize();
        }
    }
}

void parse_GlobalDecl()
{
    if (syntactic_analyser_verbosity)
        std::cout << "  > parse_GlobalDecl\n";

    Token type_token = peek();
    advance(); // Consome o tipo

    if (peek().type != TokenType::Type::IDENTIFIER)
    {
        std::cerr << "Erro Sintatico: Esperado IDENTIFIER, mas encontrado ["
                  << token_type_to_string(peek().type) << "] na linha " << peek().line << std::endl;
        syntax_error_found = true;
        synchronize();
        return;
    }

    Token name_token = peek();
    if (syntactic_analyser_verbosity)
        std::cout << "    [MATCH] IDENTIFIER ('" << name_token.text << "')\n";
    advance(); // Consome o nome

    if (peek().type == TokenType::Type::L_PAREN)
    {
        // Declaração de função
        semantic_declare(name_token.text, type_token.type, SymbolKind::FUNCTION, name_token.line);
        current_function_return_type = type_token.type;
        parse_FuncDeclTail();
    }
    else
    {
        // Declaração de variável global
        bool is_arr = (peek().type == TokenType::Type::L_BRACKET);
        semantic_declare(name_token.text, type_token.type, SymbolKind::VARIABLE, name_token.line, is_arr);
        parse_VarDeclTail(type_token.type);
    }
}

void parse_VarDeclTail(TokenType::Type expected)
{
    if (syntactic_analyser_verbosity)
        std::cout << "    > parse_VarDeclTail\n";
    if (peek().type == TokenType::Type::OP_ASSIGN)
    {
        Token assign_tok = peek();
        match(TokenType::Type::OP_ASSIGN);
        TokenType::Type init_type = parse_Expr();
        if (!is_unresolved(expected) && !is_unresolved(init_type) && expected != init_type)
        {
            semantic_report_error(
                "Tipo incompativel na inicializacao: esperado '" + type_to_string(expected) + "', encontrado '" + type_to_string(init_type) + "'",
                assign_tok.line);
        }
        match(TokenType::Type::SEMICOLON);
    }
    else if (peek().type == TokenType::Type::L_BRACKET)
    {
        match(TokenType::Type::L_BRACKET);
        match(TokenType::Type::NUMBER_LITERAL);
        match(TokenType::Type::R_BRACKET);
        match(TokenType::Type::SEMICOLON);
    }
    else
    {
        match(TokenType::Type::SEMICOLON);
    }
}

void parse_FuncDeclTail()
{
    if (syntactic_analyser_verbosity)
        std::cout << "    > parse_FuncDeclTail\n";
    match(TokenType::Type::L_PAREN);
    semantic_enter_scope(); // Parâmetros e corpo compartilham o mesmo escopo (nível 1+)
    parse_Params();
    match(TokenType::Type::R_PAREN);
    parse_Block(false); // Escopo já gerenciado por FuncDeclTail
    semantic_exit_scope();
}

// ==========================================
// PARÂMETROS E BLOCOS
// ==========================================

void parse_Params()
{
    if (syntactic_analyser_verbosity)
        std::cout << "      > parse_Params\n";
    if (is_data_type(peek().type))
    {
        parse_Param();
        parse_ParamsTail();
    }
}

void parse_Param()
{
    Token type_token = peek();
    if (is_data_type(type_token.type))
        advance();
    else
        match(TokenType::Type::TYPE_INT);

    Token name_token = peek();
    match(TokenType::Type::IDENTIFIER);
    semantic_declare(name_token.text, type_token.type, SymbolKind::VARIABLE, name_token.line);
}

void parse_ParamsTail()
{
    if (peek().type == TokenType::Type::COMMA)
    {
        match(TokenType::Type::COMMA);
        parse_Param();
        parse_ParamsTail();
    }
}

void parse_Block(bool manage_scope)
{
    if (syntactic_analyser_verbosity)
        std::cout << "  > parse_Block\n";
    match(TokenType::Type::L_BRACE);
    if (manage_scope)
        semantic_enter_scope();
    parse_Statements();
    if (manage_scope)
        semantic_exit_scope();
    match(TokenType::Type::R_BRACE);
}

// ==========================================
// STATEMENTS
// ==========================================

void parse_Statements()
{
    if (syntactic_analyser_verbosity)
        std::cout << "    > parse_Statements\n";
    while (peek().type != TokenType::Type::R_BRACE &&
           peek().type != TokenType::Type::EOF_TOKEN)
    {
        parse_Statement();
    }
}

void parse_Statement()
{
    TokenType::Type pt = peek().type;

    if (is_data_type(pt))
        parse_VarDeclStmt();
    else if (pt == TokenType::Type::KW_IF)
        parse_IfStmt();
    else if (pt == TokenType::Type::KW_WHILE)
        parse_WhileStmt();
    else if (pt == TokenType::Type::KW_FOR)
        parse_ForStmt();
    else if (pt == TokenType::Type::KW_RETURN)
        parse_ReturnStmt();
    else if (pt == TokenType::Type::KW_BREAK)
        parse_BreakStmt();
    else if (pt == TokenType::Type::KW_CONTINUE)
        parse_ContinueStmt();
    else if (pt == TokenType::Type::L_BRACE)
        parse_Block();
    else
        parse_ExprStmt();
}

void parse_VarDeclStmt()
{
    if (syntactic_analyser_verbosity)
        std::cout << "    > parse_VarDeclStmt\n";
    Token type_token = peek();
    advance(); // Tipo
    Token name_token = peek();
    match(TokenType::Type::IDENTIFIER);
    bool is_arr = (peek().type == TokenType::Type::L_BRACKET);
    semantic_declare(name_token.text, type_token.type, SymbolKind::VARIABLE, name_token.line, is_arr);
    parse_VarDeclTail(type_token.type);
}

void parse_IfStmt()
{
    if (syntactic_analyser_verbosity)
        std::cout << "    > parse_IfStmt\n";
    match(TokenType::Type::KW_IF);
    match(TokenType::Type::L_PAREN);
    parse_Expr();
    match(TokenType::Type::R_PAREN);
    parse_Statement();

    if (peek().type == TokenType::Type::KW_ELSE)
    {
        if (syntactic_analyser_verbosity)
            std::cout << "    > parse_ElsePart\n";
        match(TokenType::Type::KW_ELSE);
        parse_Statement();
    }
}

void parse_WhileStmt()
{
    if (syntactic_analyser_verbosity)
        std::cout << "    > parse_WhileStmt\n";
    match(TokenType::Type::KW_WHILE);
    match(TokenType::Type::L_PAREN);
    parse_Expr();
    match(TokenType::Type::R_PAREN);
    parse_Statement();
}

void parse_ForStmt()
{
    if (syntactic_analyser_verbosity)
        std::cout << "    > parse_ForStmt\n";
    match(TokenType::Type::KW_FOR);
    match(TokenType::Type::L_PAREN);
    parse_ForInit();
    parse_Expr();
    match(TokenType::Type::SEMICOLON);
    parse_ForUpdate();
    match(TokenType::Type::R_PAREN);
    parse_Statement();
}

void parse_ForInit()
{
    if (is_data_type(peek().type))
        parse_VarDeclStmt();
    else if (peek().type == TokenType::Type::SEMICOLON)
        match(TokenType::Type::SEMICOLON);
    else
        parse_ExprStmt();
}

void parse_ForUpdate()
{
    if (peek().type != TokenType::Type::R_PAREN)
    {
        parse_Expr();
    }
}

void parse_ReturnStmt()
{
    if (syntactic_analyser_verbosity)
        std::cout << "    > parse_ReturnStmt\n";
    match(TokenType::Type::KW_RETURN);
    parse_ReturnTail();
}

void parse_ReturnTail()
{
    if (peek().type == TokenType::Type::SEMICOLON)
    {
        match(TokenType::Type::SEMICOLON);
    }
    else
    {
        Token ret_tok = peek();
        TokenType::Type expr_type = parse_Expr();
        // Verifica tipo de retorno
        if (current_function_return_type == TokenType::Type::TYPE_VOID)
        {
            semantic_report_error("Funcao void nao pode retornar um valor", ret_tok.line);
        }
        else if (!is_unresolved(expr_type) && !is_unresolved(current_function_return_type) && current_function_return_type != expr_type)
        {
            semantic_report_error(
                "Tipo de retorno incompativel: esperado '" + type_to_string(current_function_return_type) + "', encontrado '" + type_to_string(expr_type) + "'",
                ret_tok.line);
        }
        match(TokenType::Type::SEMICOLON);
    }
}

void parse_BreakStmt()
{
    match(TokenType::Type::KW_BREAK);
    match(TokenType::Type::SEMICOLON);
}

void parse_ContinueStmt()
{
    match(TokenType::Type::KW_CONTINUE);
    match(TokenType::Type::SEMICOLON);
}

void parse_ExprStmt()
{
    if (peek().type == TokenType::Type::SEMICOLON)
    {
        match(TokenType::Type::SEMICOLON);
    }
    else
    {
        parse_Expr(); // tipo descartado em contexto de statement
        match(TokenType::Type::SEMICOLON);
    }
}

// ==========================================
// EXPRESSÕES (propagação de tipo)
// ==========================================

TokenType::Type parse_Expr()
{
    TokenType::Type left = parse_LogicalOrExpr();
    return parse_AssignTail(left);
}

TokenType::Type parse_AssignTail(TokenType::Type left_type)
{
    if (is_assign_op(peek().type))
    {
        Token op = peek();
        match(peek().type);
        TokenType::Type right = parse_Expr(); // direita-associativo
        if (!is_unresolved(left_type) && !is_unresolved(right) && left_type != right)
        {
            semantic_report_error(
                "Tipo incompativel na atribuicao: '" + type_to_string(left_type) + "' e '" + type_to_string(right) + "'",
                op.line);
        }
        return left_type;
    }
    return left_type;
}

TokenType::Type parse_LogicalOrExpr()
{
    TokenType::Type left = parse_LogicalAndExpr();
    return parse_LogicalOrTail(left);
}

TokenType::Type parse_LogicalOrTail(TokenType::Type left_type)
{
    if (peek().type == TokenType::Type::OP_OR)
    {
        Token op = peek();
        match(TokenType::Type::OP_OR);
        TokenType::Type right = parse_LogicalAndExpr();
        if (!is_unresolved(left_type) && !is_unresolved(right) && left_type != right)
        {
            semantic_report_error(
                "Tipos incompativeis no operador '||': '" + type_to_string(left_type) + "' e '" + type_to_string(right) + "'",
                op.line);
        }
        return parse_LogicalOrTail(TokenType::Type::TYPE_BOOL);
    }
    return left_type;
}

TokenType::Type parse_LogicalAndExpr()
{
    TokenType::Type left = parse_EqExpr();
    return parse_LogicalAndTail(left);
}

TokenType::Type parse_LogicalAndTail(TokenType::Type left_type)
{
    if (peek().type == TokenType::Type::OP_AND)
    {
        Token op = peek();
        match(TokenType::Type::OP_AND);
        TokenType::Type right = parse_EqExpr();
        if (!is_unresolved(left_type) && !is_unresolved(right) && left_type != right)
        {
            semantic_report_error(
                "Tipos incompativeis no operador '&&': '" + type_to_string(left_type) + "' e '" + type_to_string(right) + "'",
                op.line);
        }
        return parse_LogicalAndTail(TokenType::Type::TYPE_BOOL);
    }
    return left_type;
}

TokenType::Type parse_EqExpr()
{
    TokenType::Type left = parse_RelExpr();
    return parse_EqTail(left);
}

TokenType::Type parse_EqTail(TokenType::Type left_type)
{
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_EQ || pt == TokenType::Type::OP_NE)
    {
        Token op = peek();
        match(pt);
        TokenType::Type right = parse_RelExpr();
        if (!is_unresolved(left_type) && !is_unresolved(right) && left_type != right)
        {
            semantic_report_error(
                "Tipos incompativeis no operador de igualdade: '" + type_to_string(left_type) + "' e '" + type_to_string(right) + "'",
                op.line);
        }
        return parse_EqTail(TokenType::Type::TYPE_BOOL);
    }
    return left_type;
}

TokenType::Type parse_RelExpr()
{
    TokenType::Type left = parse_AddExpr();
    return parse_RelTail(left);
}

TokenType::Type parse_RelTail(TokenType::Type left_type)
{
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_LT || pt == TokenType::Type::OP_GT ||
        pt == TokenType::Type::OP_LE || pt == TokenType::Type::OP_GE)
    {
        Token op = peek();
        match(pt);
        TokenType::Type right = parse_AddExpr();
        if (!is_unresolved(left_type) && !is_unresolved(right) && left_type != right)
        {
            semantic_report_error(
                "Tipos incompativeis no operador relacional: '" + type_to_string(left_type) + "' e '" + type_to_string(right) + "'",
                op.line);
        }
        return parse_RelTail(TokenType::Type::TYPE_BOOL);
    }
    return left_type;
}

TokenType::Type parse_AddExpr()
{
    TokenType::Type left = parse_MultExpr();
    return parse_AddTail(left);
}

TokenType::Type parse_AddTail(TokenType::Type left_type)
{
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_PLUS || pt == TokenType::Type::OP_MINUS)
    {
        Token op = peek();
        match(pt);
        TokenType::Type right = parse_MultExpr();
        if (!is_unresolved(left_type) && !is_unresolved(right) && left_type != right)
        {
            semantic_report_error(
                "Tipos incompativeis na operacao aritmetica: '" + type_to_string(left_type) + "' e '" + type_to_string(right) + "'",
                op.line);
        }
        TokenType::Type result = is_unresolved(left_type) ? right : left_type;
        return parse_AddTail(result);
    }
    return left_type;
}

TokenType::Type parse_MultExpr()
{
    TokenType::Type left = parse_UnaryExpr();
    return parse_MultTail(left);
}

TokenType::Type parse_MultTail(TokenType::Type left_type)
{
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_MULT || pt == TokenType::Type::OP_DIV || pt == TokenType::Type::OP_MOD)
    {
        Token op = peek();
        match(pt);
        TokenType::Type right = parse_UnaryExpr();
        if (!is_unresolved(left_type) && !is_unresolved(right) && left_type != right)
        {
            semantic_report_error(
                "Tipos incompativeis na operacao aritmetica: '" + type_to_string(left_type) + "' e '" + type_to_string(right) + "'",
                op.line);
        }
        TokenType::Type result = is_unresolved(left_type) ? right : left_type;
        return parse_MultTail(result);
    }
    return left_type;
}

TokenType::Type parse_UnaryExpr()
{
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_MINUS || pt == TokenType::Type::OP_NOT ||
        pt == TokenType::Type::OP_INC || pt == TokenType::Type::OP_DEC)
    {
        Token op = peek();
        match(pt);
        TokenType::Type inner = parse_UnaryExpr();
        if (op.type == TokenType::Type::OP_NOT)
            return TokenType::Type::TYPE_BOOL;
        return inner;
    }
    else
    {
        TokenType::Type primary_type = parse_PrimaryExpr();
        return parse_PostfixTail(primary_type);
    }
}

TokenType::Type parse_PrimaryExpr()
{
    TokenType::Type pt = peek().type;

    if (pt == TokenType::Type::IDENTIFIER)
    {
        Token id_token = peek();
        match(TokenType::Type::IDENTIFIER);
        bool is_func_call = (peek().type == TokenType::Type::L_PAREN);
        Symbol *sym = semantic_lookup(id_token.text);
        if (!sym)
        {
            if (is_func_call)
            {
                semantic_report_error("Funcao nao declarada: '" + id_token.text + "'", id_token.line);
            }
            else
            {
                semantic_report_error("Variavel nao declarada: '" + id_token.text + "'", id_token.line);
            }
            return TYPE_UNRESOLVED;
        }
        return sym->type;
    }
    else if (pt == TokenType::Type::NUMBER_LITERAL)
    {
        Token num = peek();
        match(TokenType::Type::NUMBER_LITERAL);
        bool is_float = (num.text.find('.') != std::string::npos);
        return is_float ? TokenType::Type::TYPE_FLOAT : TokenType::Type::TYPE_INT;
    }
    else if (pt == TokenType::Type::STRING_LITERAL)
    {
        match(TokenType::Type::STRING_LITERAL);
        return TokenType::Type::TYPE_STRING;
    }
    else if (pt == TokenType::Type::L_PAREN)
    {
        match(TokenType::Type::L_PAREN);
        TokenType::Type t = parse_Expr();
        match(TokenType::Type::R_PAREN);
        return t;
    }
    else
    {
        std::cerr << "Erro Sintatico: Esperado Expressao, mas encontrado ["
                  << token_type_to_string(pt) << "] ('" << peek().text << "') na linha "
                  << peek().line << std::endl;
        syntax_error_found = true;

        if (pt != TokenType::Type::SEMICOLON &&
            pt != TokenType::Type::R_BRACE &&
            pt != TokenType::Type::EOF_TOKEN &&
            pt != TokenType::Type::R_PAREN &&
            pt != TokenType::Type::R_BRACKET)
        {
            advance();
        }
        synchronize();
        return TYPE_UNRESOLVED;
    }
}

TokenType::Type parse_PostfixTail(TokenType::Type base_type)
{
    TokenType::Type pt = peek().type;

    if (pt == TokenType::Type::L_PAREN)
    {
        match(TokenType::Type::L_PAREN);
        parse_Args();
        match(TokenType::Type::R_PAREN);
        return parse_PostfixTail(base_type); // tipo de retorno da função
    }
    else if (pt == TokenType::Type::L_BRACKET)
    {
        match(TokenType::Type::L_BRACKET);
        Token idx_tok = peek();
        TokenType::Type idx_type = parse_Expr();
        if (!is_unresolved(idx_type) && idx_type != TokenType::Type::TYPE_INT)
        {
            semantic_report_error("Indice de array deve ser do tipo 'int'", idx_tok.line);
        }
        match(TokenType::Type::R_BRACKET);
        return parse_PostfixTail(base_type); // elemento tem o tipo base do array
    }
    else if (pt == TokenType::Type::OP_INC || pt == TokenType::Type::OP_DEC)
    {
        match(pt);
        return parse_PostfixTail(base_type);
    }

    return base_type;
}

void parse_Args()
{
    if (peek().type != TokenType::Type::R_PAREN)
    {
        parse_Expr();
        parse_ArgsTail();
    }
}

void parse_ArgsTail()
{
    if (peek().type == TokenType::Type::COMMA)
    {
        match(TokenType::Type::COMMA);
        parse_Expr();
        parse_ArgsTail();
    }
}

// ==========================================
// PONTO DE ENTRADA DO ANALISADOR SINTÁTICO
// ==========================================

bool syntactic_analyser(std::vector<Token> &tokens, bool verbose)
{
    global_tokens = tokens;
    current_token_index = 0;
    syntax_error_found = false;
    syntactic_analyser_verbosity = verbose;

    if (syntactic_analyser_verbosity)
    {
        std::cout << "\n=== INICIANDO ANALISE SINTATICA (TOP-DOWN) ===\n";
    }

    parse_Program();

    return !syntax_error_found;
}
