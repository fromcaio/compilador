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
        if (codegen_enabled) codegen_func_begin(name_token.text);
        parse_FuncDeclTail();
        if (codegen_enabled) codegen_func_end();
    }
    else
    {
        // Declaração de variável global
        bool is_arr = (peek().type == TokenType::Type::L_BRACKET);
        // Capturar tamanho do array para codegen (TOKEN: [ NUMBER ] ;)
        int array_size = 0;
        if (is_arr)
        {
            // Peek à frente: L_BRACKET → NUMBER_LITERAL → R_BRACKET
            if (current_token_index + 1 < global_tokens.size())
                array_size = std::stoi(global_tokens[current_token_index + 1].text);
        }
        semantic_declare(name_token.text, type_token.type, SymbolKind::VARIABLE, name_token.line, is_arr);
        if (codegen_enabled)
            codegen_global_var(name_token.text, type_token.type, is_arr, array_size);
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
        ExprResult init = parse_Expr();
        if (!is_unresolved(expected) && !is_unresolved(init.type) && expected != init.type)
        {
            semantic_report_error(
                "Tipo incompativel na inicializacao: esperado '" + type_to_string(expected) + "', encontrado '" + type_to_string(init.type) + "'",
                assign_tok.line);
        }
        if (codegen_enabled && !init.reg.empty())
            codegen_store_var(codegen_last_decl_name, init.reg, expected);
        codegen_free_reg(init.reg);
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

static int s_param_index = 0; // contador de parâmetro da função atual

void parse_Params()
{
    if (syntactic_analyser_verbosity)
        std::cout << "      > parse_Params\n";
    if (is_data_type(peek().type))
    {
        s_param_index = 0;
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
    if (codegen_enabled) codegen_param(name_token.text, type_token.type, s_param_index);
    ++s_param_index;
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
    if (codegen_enabled && !is_arr)
        codegen_alloc_local(name_token.text, type_token.type);
    codegen_last_decl_name = name_token.text;
    parse_VarDeclTail(type_token.type);
}

void parse_IfStmt()
{
    if (syntactic_analyser_verbosity)
        std::cout << "    > parse_IfStmt\n";
    match(TokenType::Type::KW_IF);
    match(TokenType::Type::L_PAREN);
    ExprResult cond = parse_Expr();
    match(TokenType::Type::R_PAREN);

    std::string lbl_else = codegen_new_label("else");
    std::string lbl_end  = codegen_new_label("end_if");

    if (codegen_enabled && !cond.reg.empty())
        codegen_emit("    beqz " + cond.reg + ", " + lbl_else);
    codegen_free_reg(cond.reg);

    parse_Statement(); // then

    if (codegen_enabled) codegen_emit("    j    " + lbl_end);
    if (codegen_enabled) codegen_emit(lbl_else + ":");

    if (peek().type == TokenType::Type::KW_ELSE)
    {
        if (syntactic_analyser_verbosity)
            std::cout << "    > parse_ElsePart\n";
        match(TokenType::Type::KW_ELSE);
        parse_Statement();
    }

    if (codegen_enabled) codegen_emit(lbl_end + ":");
}

void parse_WhileStmt()
{
    if (syntactic_analyser_verbosity)
        std::cout << "    > parse_WhileStmt\n";

    std::string lbl_cond = codegen_new_label("while_cond");
    std::string lbl_end  = codegen_new_label("while_end");

    if (codegen_enabled) codegen_emit(lbl_cond + ":");

    match(TokenType::Type::KW_WHILE);
    match(TokenType::Type::L_PAREN);
    ExprResult cond = parse_Expr();
    match(TokenType::Type::R_PAREN);

    if (codegen_enabled && !cond.reg.empty())
        codegen_emit("    beqz " + cond.reg + ", " + lbl_end);
    codegen_free_reg(cond.reg);

    if (codegen_enabled) codegen_loop_stack.push_back({lbl_cond, lbl_end});
    parse_Statement();
    if (codegen_enabled) codegen_loop_stack.pop_back();

    if (codegen_enabled)
    {
        codegen_emit("    j    " + lbl_cond);
        codegen_emit(lbl_end + ":");
    }
}

void parse_ForStmt()
{
    if (syntactic_analyser_verbosity)
        std::cout << "    > parse_ForStmt\n";

    std::string lbl_cond   = codegen_new_label("for_cond");
    std::string lbl_update = codegen_new_label("for_update");
    std::string lbl_end    = codegen_new_label("for_end");

    match(TokenType::Type::KW_FOR);
    match(TokenType::Type::L_PAREN);

    parse_ForInit();

    if (codegen_enabled) codegen_emit(lbl_cond + ":");

    // Condição (opcional: se ';' imediato, loop infinito)
    if (peek().type != TokenType::Type::SEMICOLON)
    {
        ExprResult cond = parse_Expr();
        if (codegen_enabled && !cond.reg.empty())
            codegen_emit("    beqz " + cond.reg + ", " + lbl_end);
        codegen_free_reg(cond.reg);
    }
    match(TokenType::Type::SEMICOLON);

    // Pula o update na primeira iteração: vai direto ao corpo
    if (codegen_enabled) codegen_emit("    j    " + lbl_update + "_body");

    if (codegen_enabled) codegen_emit(lbl_update + ":");
    parse_ForUpdate();
    if (codegen_enabled) codegen_emit("    j    " + lbl_cond);

    if (codegen_enabled) codegen_emit(lbl_update + "_body:");

    match(TokenType::Type::R_PAREN);

    if (codegen_enabled) codegen_loop_stack.push_back({lbl_update, lbl_end});
    parse_Statement();
    if (codegen_enabled) codegen_loop_stack.pop_back();

    if (codegen_enabled)
    {
        codegen_emit("    j    " + lbl_update);
        codegen_emit(lbl_end + ":");
    }
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
        ExprResult r = parse_Expr();
        codegen_free_reg(r.reg);
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
        if (codegen_enabled) codegen_emit("    j    " + codegen_func_end_label);
    }
    else
    {
        Token ret_tok = peek();
        ExprResult val = parse_Expr();
        // Verifica tipo de retorno
        if (current_function_return_type == TokenType::Type::TYPE_VOID)
        {
            semantic_report_error("Funcao void nao pode retornar um valor", ret_tok.line);
        }
        else if (!is_unresolved(val.type) && !is_unresolved(current_function_return_type) && current_function_return_type != val.type)
        {
            semantic_report_error(
                "Tipo de retorno incompativel: esperado '" + type_to_string(current_function_return_type) + "', encontrado '" + type_to_string(val.type) + "'",
                ret_tok.line);
        }
        if (codegen_enabled && !val.reg.empty())
        {
            if (codegen_is_float_type(current_function_return_type))
                codegen_emit("    fmv.s fa0, " + val.reg);
            else
                codegen_emit("    mv   a0, " + val.reg);
            codegen_free_reg(val.reg);
        }
        if (codegen_enabled) codegen_emit("    j    " + codegen_func_end_label);
        match(TokenType::Type::SEMICOLON);
    }
}

void parse_BreakStmt()
{
    Token tok = peek();
    match(TokenType::Type::KW_BREAK);
    if (codegen_enabled)
    {
        if (codegen_loop_stack.empty())
            semantic_report_error("'break' fora de um laco", tok.line);
        else
            codegen_emit("    j    " + codegen_loop_stack.back().break_label);
    }
    match(TokenType::Type::SEMICOLON);
}

void parse_ContinueStmt()
{
    Token tok = peek();
    match(TokenType::Type::KW_CONTINUE);
    if (codegen_enabled)
    {
        if (codegen_loop_stack.empty())
            semantic_report_error("'continue' fora de um laco", tok.line);
        else
            codegen_emit("    j    " + codegen_loop_stack.back().continue_label);
    }
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
        ExprResult r = parse_Expr();
        codegen_free_reg(r.reg);
        // Libera endereço de array pendente (caso expr seja arr[i] sem assignment)
        if (!codegen_lhs_addr_reg.empty())
        {
            codegen_free_reg(codegen_lhs_addr_reg);
            codegen_lhs_addr_reg = "";
        }
        match(TokenType::Type::SEMICOLON);
    }
}

// ==========================================
// EXPRESSÕES (propagação de tipo + codegen)
// ==========================================

ExprResult parse_Expr()
{
    ExprResult left = parse_LogicalOrExpr();
    return parse_AssignTail(left);
}

ExprResult parse_AssignTail(ExprResult left)
{
    if (is_assign_op(peek().type))
    {
        Token op = peek();
        match(peek().type);

        // Captura LHS antes de parsear RHS (RHS vai sobrescrever os globais)
        std::string lhs_name = codegen_last_lhs_name;
        std::string lhs_addr = codegen_lhs_addr_reg;
        codegen_lhs_addr_reg = ""; // limpa para RHS não interferir
        // O valor carregado do LHS não é necessário — libera o registrador agora
        codegen_free_reg(left.reg);

        ExprResult right = parse_Expr(); // direita-associativo
        if (!is_unresolved(left.type) && !is_unresolved(right.type) && left.type != right.type)
        {
            semantic_report_error(
                "Tipo incompativel na atribuicao: '" + type_to_string(left.type) + "' e '" + type_to_string(right.type) + "'",
                op.line);
        }
        if (codegen_enabled && !right.reg.empty())
        {
            bool is_float = codegen_is_float_type(left.type);
            std::string store_instr = is_float ? "fsw" : "sw";

            if (op.type != TokenType::Type::OP_ASSIGN && !lhs_addr.empty())
            {
                // Compound assignment a array[i] += rhs
                std::string cur = is_float ? codegen_alloc_freg() : codegen_alloc_ireg();
                codegen_emit("    " + std::string(is_float ? "flw" : "lw") +
                             "  " + cur + ", 0(" + lhs_addr + ")");
                std::string res = is_float ? codegen_alloc_freg() : codegen_alloc_ireg();
                std::string istr;
                if (op.type == TokenType::Type::OP_PLUS_ASSIGN)  istr = is_float ? "fadd.s" : "add";
                else if (op.type == TokenType::Type::OP_MINUS_ASSIGN) istr = is_float ? "fsub.s" : "sub";
                else if (op.type == TokenType::Type::OP_MULT_ASSIGN)  istr = is_float ? "fmul.s" : "mul";
                else if (op.type == TokenType::Type::OP_DIV_ASSIGN)   istr = is_float ? "fdiv.s" : "div";
                else if (op.type == TokenType::Type::OP_MOD_ASSIGN)   istr = "rem";
                if (!istr.empty())
                    codegen_emit("    " + istr + " " + res + ", " + cur + ", " + right.reg);
                codegen_free_reg(cur);
                codegen_free_reg(right.reg);
                codegen_emit("    " + store_instr + "  " + res + ", 0(" + lhs_addr + ")");
                codegen_free_reg(res);
                codegen_free_reg(lhs_addr);
            }
            else if (op.type != TokenType::Type::OP_ASSIGN && !lhs_name.empty())
            {
                // Compound assignment a variável
                ExprResult cur = codegen_load_var(lhs_name, left.type);
                std::string res = is_float ? codegen_alloc_freg() : codegen_alloc_ireg();
                std::string istr;
                if (op.type == TokenType::Type::OP_PLUS_ASSIGN)  istr = is_float ? "fadd.s" : "add";
                else if (op.type == TokenType::Type::OP_MINUS_ASSIGN) istr = is_float ? "fsub.s" : "sub";
                else if (op.type == TokenType::Type::OP_MULT_ASSIGN)  istr = is_float ? "fmul.s" : "mul";
                else if (op.type == TokenType::Type::OP_DIV_ASSIGN)   istr = is_float ? "fdiv.s" : "div";
                else if (op.type == TokenType::Type::OP_MOD_ASSIGN)   istr = "rem";
                if (!istr.empty())
                    codegen_emit("    " + istr + " " + res + ", " + cur.reg + ", " + right.reg);
                codegen_free_reg(cur.reg);
                codegen_free_reg(right.reg);
                codegen_store_var(lhs_name, res, left.type);
                codegen_free_reg(res);
            }
            else if (!lhs_addr.empty())
            {
                // Simple assignment a array[i]
                codegen_emit("    " + store_instr + "  " + right.reg + ", 0(" + lhs_addr + ")");
                codegen_free_reg(right.reg);
                codegen_free_reg(lhs_addr);
            }
            else
            {
                // Simple assignment a variável
                codegen_store_var(lhs_name, right.reg, left.type);
                codegen_free_reg(right.reg);
            }
        }
        else
        {
            if (!lhs_addr.empty()) codegen_free_reg(lhs_addr);
            codegen_free_reg(right.reg);
        }
        return left;
    }
    // Não é assignment: liberar addr reservado se existir
    if (!codegen_lhs_addr_reg.empty())
    {
        codegen_free_reg(codegen_lhs_addr_reg);
        codegen_lhs_addr_reg = "";
    }
    return left;
}

ExprResult parse_LogicalOrExpr()
{
    ExprResult left = parse_LogicalAndExpr();
    return parse_LogicalOrTail(left);
}

ExprResult parse_LogicalOrTail(ExprResult left)
{
    if (peek().type == TokenType::Type::OP_OR)
    {
        Token op = peek();
        match(TokenType::Type::OP_OR);
        ExprResult right = parse_LogicalAndExpr();
        if (!is_unresolved(left.type) && !is_unresolved(right.type) && left.type != right.type)
        {
            semantic_report_error(
                "Tipos incompativeis no operador '||': '" + type_to_string(left.type) + "' e '" + type_to_string(right.type) + "'",
                op.line);
        }
        std::string res = "";
        if (codegen_enabled && !left.reg.empty() && !right.reg.empty())
        {
            res = codegen_alloc_ireg();
            codegen_emit("    or   " + res + ", " + left.reg + ", " + right.reg);
        }
        codegen_free_reg(left.reg);
        codegen_free_reg(right.reg);
        return parse_LogicalOrTail({TokenType::Type::TYPE_BOOL, res});
    }
    return left;
}

ExprResult parse_LogicalAndExpr()
{
    ExprResult left = parse_EqExpr();
    return parse_LogicalAndTail(left);
}

ExprResult parse_LogicalAndTail(ExprResult left)
{
    if (peek().type == TokenType::Type::OP_AND)
    {
        Token op = peek();
        match(TokenType::Type::OP_AND);
        ExprResult right = parse_EqExpr();
        if (!is_unresolved(left.type) && !is_unresolved(right.type) && left.type != right.type)
        {
            semantic_report_error(
                "Tipos incompativeis no operador '&&': '" + type_to_string(left.type) + "' e '" + type_to_string(right.type) + "'",
                op.line);
        }
        std::string res = "";
        if (codegen_enabled && !left.reg.empty() && !right.reg.empty())
        {
            res = codegen_alloc_ireg();
            codegen_emit("    and  " + res + ", " + left.reg + ", " + right.reg);
        }
        codegen_free_reg(left.reg);
        codegen_free_reg(right.reg);
        return parse_LogicalAndTail({TokenType::Type::TYPE_BOOL, res});
    }
    return left;
}

ExprResult parse_EqExpr()
{
    ExprResult left = parse_RelExpr();
    return parse_EqTail(left);
}

ExprResult parse_EqTail(ExprResult left)
{
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_EQ || pt == TokenType::Type::OP_NE)
    {
        Token op = peek();
        match(pt);
        ExprResult right = parse_RelExpr();
        if (!is_unresolved(left.type) && !is_unresolved(right.type) && left.type != right.type)
        {
            semantic_report_error(
                "Tipos incompativeis no operador de igualdade: '" + type_to_string(left.type) + "' e '" + type_to_string(right.type) + "'",
                op.line);
        }
        std::string res = "";
        if (codegen_enabled && !left.reg.empty() && !right.reg.empty())
        {
            res = codegen_alloc_ireg();
            codegen_emit("    sub  " + res + ", " + left.reg + ", " + right.reg);
            if (pt == TokenType::Type::OP_EQ)
                codegen_emit("    seqz " + res + ", " + res);
            else
                codegen_emit("    snez " + res + ", " + res);
        }
        codegen_free_reg(left.reg);
        codegen_free_reg(right.reg);
        return parse_EqTail({TokenType::Type::TYPE_BOOL, res});
    }
    return left;
}

ExprResult parse_RelExpr()
{
    ExprResult left = parse_AddExpr();
    return parse_RelTail(left);
}

ExprResult parse_RelTail(ExprResult left)
{
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_LT || pt == TokenType::Type::OP_GT ||
        pt == TokenType::Type::OP_LE || pt == TokenType::Type::OP_GE)
    {
        Token op = peek();
        match(pt);
        ExprResult right = parse_AddExpr();
        if (!is_unresolved(left.type) && !is_unresolved(right.type) && left.type != right.type)
        {
            semantic_report_error(
                "Tipos incompativeis no operador relacional: '" + type_to_string(left.type) + "' e '" + type_to_string(right.type) + "'",
                op.line);
        }
        std::string res = "";
        if (codegen_enabled && !left.reg.empty() && !right.reg.empty())
        {
            res = codegen_alloc_ireg();
            if (pt == TokenType::Type::OP_LT)
                codegen_emit("    slt  " + res + ", " + left.reg + ", " + right.reg);
            else if (pt == TokenType::Type::OP_GT)
                codegen_emit("    slt  " + res + ", " + right.reg + ", " + left.reg);
            else if (pt == TokenType::Type::OP_LE)
            {
                codegen_emit("    slt  " + res + ", " + right.reg + ", " + left.reg);
                codegen_emit("    xori " + res + ", " + res + ", 1");
            }
            else // GE
            {
                codegen_emit("    slt  " + res + ", " + left.reg + ", " + right.reg);
                codegen_emit("    xori " + res + ", " + res + ", 1");
            }
        }
        codegen_free_reg(left.reg);
        codegen_free_reg(right.reg);
        return parse_RelTail({TokenType::Type::TYPE_BOOL, res});
    }
    return left;
}

ExprResult parse_AddExpr()
{
    ExprResult left = parse_MultExpr();
    return parse_AddTail(left);
}

ExprResult parse_AddTail(ExprResult left)
{
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_PLUS || pt == TokenType::Type::OP_MINUS)
    {
        Token op = peek();
        match(pt);
        ExprResult right = parse_MultExpr();
        if (!is_unresolved(left.type) && !is_unresolved(right.type) && left.type != right.type)
        {
            semantic_report_error(
                "Tipos incompativeis na operacao aritmetica: '" + type_to_string(left.type) + "' e '" + type_to_string(right.type) + "'",
                op.line);
        }
        TokenType::Type res_type = is_unresolved(left.type) ? right.type : left.type;
        std::string res = "";
        if (codegen_enabled && !left.reg.empty() && !right.reg.empty())
        {
            bool is_float = codegen_is_float_type(res_type);
            res = is_float ? codegen_alloc_freg() : codegen_alloc_ireg();
            bool is_sub = (pt == TokenType::Type::OP_MINUS);
            if (is_float)
                codegen_emit(is_sub
                    ? "    fsub.s " + res + ", " + left.reg + ", " + right.reg
                    : "    fadd.s " + res + ", " + left.reg + ", " + right.reg);
            else
                codegen_emit(is_sub
                    ? "    sub  " + res + ", " + left.reg + ", " + right.reg
                    : "    add  " + res + ", " + left.reg + ", " + right.reg);
        }
        codegen_free_reg(left.reg);
        codegen_free_reg(right.reg);
        return parse_AddTail({res_type, res});
    }
    return left;
}

ExprResult parse_MultExpr()
{
    ExprResult left = parse_UnaryExpr();
    return parse_MultTail(left);
}

ExprResult parse_MultTail(ExprResult left)
{
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_MULT || pt == TokenType::Type::OP_DIV || pt == TokenType::Type::OP_MOD)
    {
        Token op = peek();
        match(pt);
        ExprResult right = parse_UnaryExpr();
        if (!is_unresolved(left.type) && !is_unresolved(right.type) && left.type != right.type)
        {
            semantic_report_error(
                "Tipos incompativeis na operacao aritmetica: '" + type_to_string(left.type) + "' e '" + type_to_string(right.type) + "'",
                op.line);
        }
        TokenType::Type res_type = is_unresolved(left.type) ? right.type : left.type;
        std::string res = "";
        if (codegen_enabled && !left.reg.empty() && !right.reg.empty())
        {
            bool is_float = codegen_is_float_type(res_type);
            res = is_float ? codegen_alloc_freg() : codegen_alloc_ireg();
            if (pt == TokenType::Type::OP_MULT)
                codegen_emit(is_float
                    ? "    fmul.s " + res + ", " + left.reg + ", " + right.reg
                    : "    mul  " + res + ", " + left.reg + ", " + right.reg);
            else if (pt == TokenType::Type::OP_DIV)
                codegen_emit(is_float
                    ? "    fdiv.s " + res + ", " + left.reg + ", " + right.reg
                    : "    div  " + res + ", " + left.reg + ", " + right.reg);
            else // MOD
                codegen_emit("    rem  " + res + ", " + left.reg + ", " + right.reg);
        }
        codegen_free_reg(left.reg);
        codegen_free_reg(right.reg);
        return parse_MultTail({res_type, res});
    }
    return left;
}

ExprResult parse_UnaryExpr()
{
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_MINUS || pt == TokenType::Type::OP_NOT ||
        pt == TokenType::Type::OP_INC || pt == TokenType::Type::OP_DEC)
    {
        Token op = peek();
        match(pt);
        ExprResult inner = parse_UnaryExpr();
        if (op.type == TokenType::Type::OP_NOT)
        {
            std::string res = "";
            if (codegen_enabled && !inner.reg.empty())
            {
                res = codegen_alloc_ireg();
                codegen_emit("    seqz " + res + ", " + inner.reg);
                codegen_free_reg(inner.reg);
            }
            return {TokenType::Type::TYPE_BOOL, res};
        }
        if (op.type == TokenType::Type::OP_MINUS && codegen_enabled && !inner.reg.empty())
        {
            bool is_float = codegen_is_float_type(inner.type);
            std::string res = is_float ? codegen_alloc_freg() : codegen_alloc_ireg();
            if (is_float)
                codegen_emit("    fneg.s " + res + ", " + inner.reg);
            else
                codegen_emit("    neg  " + res + ", " + inner.reg);
            codegen_free_reg(inner.reg);
            return {inner.type, res};
        }
        if ((op.type == TokenType::Type::OP_INC || op.type == TokenType::Type::OP_DEC)
            && codegen_enabled && !inner.reg.empty() && !codegen_last_lhs_name.empty())
        {
            // Pré-inc/dec: modifica e carrega
            std::string tmp = codegen_alloc_ireg();
            codegen_emit("    addi " + tmp + ", " + inner.reg + ", " +
                         (op.type == TokenType::Type::OP_INC ? "1" : "-1"));
            codegen_store_var(codegen_last_lhs_name, tmp, inner.type);
            codegen_free_reg(inner.reg);
            return {inner.type, tmp};
        }
        return inner;
    }
    else
    {
        ExprResult primary = parse_PrimaryExpr();
        return parse_PostfixTail(primary);
    }
}

ExprResult parse_PrimaryExpr()
{
    TokenType::Type pt = peek().type;

    if (pt == TokenType::Type::IDENTIFIER)
    {
        Token id_token = peek();
        match(TokenType::Type::IDENTIFIER);
        bool is_func_call = (peek().type == TokenType::Type::L_PAREN);
        codegen_last_lhs_name = id_token.text; // salva para AssignTail / postfix inc/dec
        Symbol *sym = semantic_lookup(id_token.text);
        if (!sym)
        {
            if (is_func_call)
                semantic_report_error("Funcao nao declarada: '" + id_token.text + "'", id_token.line);
            else
                semantic_report_error("Variavel nao declarada: '" + id_token.text + "'", id_token.line);
            return {TYPE_UNRESOLVED, ""};
        }
        if (!is_func_call)
        {
            if (codegen_enabled)
            {
                if (sym->is_array)
                {
                    // Array: retorna endereço base (la), não o valor
                    std::string reg = codegen_alloc_ireg();
                    codegen_emit("    la   " + reg + ", " + sym->name);
                    return {sym->type, reg};
                }
                return codegen_load_var(sym->name, sym->type);
            }
        }
        return {sym->type, ""};
    }
    else if (pt == TokenType::Type::NUMBER_LITERAL)
    {
        Token num = peek();
        match(TokenType::Type::NUMBER_LITERAL);
        bool is_float = (num.text.find('.') != std::string::npos);
        if (!codegen_enabled)
            return {is_float ? TokenType::Type::TYPE_FLOAT : TokenType::Type::TYPE_INT, ""};

        if (is_float)
        {
            // Constante float: emite em .rodata e carrega
            std::string lbl = codegen_new_label("fconst");
            codegen_emit_global(".section .rodata");
            codegen_emit_global(lbl + ": .float " + num.text);
            codegen_emit_global(".text");
            std::string addr = codegen_alloc_ireg();
            std::string freg = codegen_alloc_freg();
            codegen_emit("    la   " + addr + ", " + lbl);
            codegen_emit("    flw  " + freg + ", 0(" + addr + ")");
            codegen_free_reg(addr);
            return {TokenType::Type::TYPE_FLOAT, freg};
        }
        else
        {
            std::string reg = codegen_alloc_ireg();
            codegen_emit("    li   " + reg + ", " + num.text);
            return {TokenType::Type::TYPE_INT, reg};
        }
    }
    else if (pt == TokenType::Type::STRING_LITERAL)
    {
        Token str = peek();
        match(TokenType::Type::STRING_LITERAL);
        if (!codegen_enabled)
            return {TokenType::Type::TYPE_STRING, ""};
        // Emite string em .rodata, retorna ponteiro
        std::string lbl = codegen_new_label("strconst");
        std::string raw = str.text; // inclui as aspas
        codegen_emit_global(".section .rodata");
        codegen_emit_global(lbl + ": .string " + raw);
        codegen_emit_global(".text");
        std::string reg = codegen_alloc_ireg();
        codegen_emit("    la   " + reg + ", " + lbl);
        return {TokenType::Type::TYPE_STRING, reg};
    }
    else if (pt == TokenType::Type::L_PAREN)
    {
        match(TokenType::Type::L_PAREN);
        ExprResult t = parse_Expr();
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
        return {TYPE_UNRESOLVED, ""};
    }
}

ExprResult parse_PostfixTail(ExprResult base)
{
    TokenType::Type pt = peek().type;

    if (pt == TokenType::Type::L_PAREN)
    {
        // Chamada de função: nome está em codegen_last_lhs_name
        std::string func_name = codegen_last_lhs_name;
        match(TokenType::Type::L_PAREN);
        codegen_arg_index = 0;
        parse_Args();
        match(TokenType::Type::R_PAREN);

        std::string res = "";
        if (codegen_enabled)
        {
            codegen_emit("    jal  ra, " + func_name);
            if (base.type != TokenType::Type::TYPE_VOID)
            {
                if (codegen_is_float_type(base.type))
                {
                    res = codegen_alloc_freg();
                    codegen_emit("    fmv.s " + res + ", fa0");
                }
                else
                {
                    res = codegen_alloc_ireg();
                    codegen_emit("    mv   " + res + ", a0");
                }
            }
        }
        return parse_PostfixTail({base.type, res});
    }
    else if (pt == TokenType::Type::L_BRACKET)
    {
        match(TokenType::Type::L_BRACKET);
        Token idx_tok = peek();
        ExprResult idx = parse_Expr();
        if (!is_unresolved(idx.type) && idx.type != TokenType::Type::TYPE_INT)
        {
            semantic_report_error("Indice de array deve ser do tipo 'int'", idx_tok.line);
        }
        match(TokenType::Type::R_BRACKET);

        std::string res = "";
        if (codegen_enabled && !idx.reg.empty())
        {
            // base.reg já é o endereço base do array (carregado por la em PrimaryExpr)
            std::string addr = base.reg.empty() ? codegen_alloc_ireg() : base.reg;
            if (base.reg.empty())
                codegen_emit("    la   " + addr + ", " + codegen_last_lhs_name);

            std::string scaled = codegen_alloc_ireg();
            codegen_emit("    slli " + scaled + ", " + idx.reg + ", 2");
            codegen_emit("    add  " + addr + ", " + addr + ", " + scaled);
            codegen_free_reg(scaled);
            codegen_free_reg(idx.reg);

            // Salva endereço como potencial LHS (para atribuição a array[i])
            // Não libera addr aqui; parse_AssignTail ou caller faz o free
            codegen_lhs_addr_reg = addr;

            if (codegen_is_float_type(base.type))
            {
                res = codegen_alloc_freg();
                codegen_emit("    flw  " + res + ", 0(" + addr + ")");
            }
            else
            {
                res = codegen_alloc_ireg();
                codegen_emit("    lw   " + res + ", 0(" + addr + ")");
            }
            // addr mantido em codegen_lhs_addr_reg; será liberado por AssignTail ou ExprStmt
        }
        return parse_PostfixTail({base.type, res});
    }
    else if (pt == TokenType::Type::OP_INC || pt == TokenType::Type::OP_DEC)
    {
        Token op = peek();
        match(pt);
        // Pós-inc/dec: retorna valor antigo, depois incrementa
        std::string old_val = base.reg;
        if (codegen_enabled && !base.reg.empty() && !codegen_last_lhs_name.empty())
        {
            std::string updated = codegen_alloc_ireg();
            codegen_emit("    addi " + updated + ", " + base.reg + ", " +
                         (op.type == TokenType::Type::OP_INC ? "1" : "-1"));
            codegen_store_var(codegen_last_lhs_name, updated, base.type);
            codegen_free_reg(updated);
            // Retorna valor original (base.reg) — não libera aqui, caller faz free
        }
        return parse_PostfixTail({base.type, old_val});
    }

    return base;
}

void parse_Args()
{
    if (peek().type != TokenType::Type::R_PAREN)
    {
        ExprResult r = parse_Expr();
        if (codegen_enabled && !r.reg.empty())
        {
            std::string areg = codegen_is_float_type(r.type)
                ? "fa" + std::to_string(codegen_arg_index)
                : "a"  + std::to_string(codegen_arg_index);
            std::string mv = codegen_is_float_type(r.type) ? "fmv.s" : "mv";
            codegen_emit("    " + mv + "  " + areg + ", " + r.reg);
            codegen_free_reg(r.reg);
        }
        ++codegen_arg_index;
        parse_ArgsTail();
    }
}

void parse_ArgsTail()
{
    if (peek().type == TokenType::Type::COMMA)
    {
        match(TokenType::Type::COMMA);
        ExprResult r = parse_Expr();
        if (codegen_enabled && !r.reg.empty())
        {
            std::string areg = codegen_is_float_type(r.type)
                ? "fa" + std::to_string(codegen_arg_index)
                : "a"  + std::to_string(codegen_arg_index);
            std::string mv = codegen_is_float_type(r.type) ? "fmv.s" : "mv";
            codegen_emit("    " + mv + "  " + areg + ", " + r.reg);
            codegen_free_reg(r.reg);
        }
        ++codegen_arg_index;
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
