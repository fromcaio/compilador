//
// Created by fc on 4/21/26.
//
#include "../HeaderFiles/syntactic_analyser.h"
#include <iostream>

static size_t current_token_index;
static std::vector<Token> global_tokens;
static bool syntax_error_found;
static syntax_tree s_tree;
static bool syntactic_analyser_verbosity;

// ==========================================
// FUNÇÕES UTILITÁRIAS LÉXICAS
// ==========================================

Token peek() {
    if (current_token_index < global_tokens.size()) {
        return global_tokens[current_token_index];
    }
    // Retorno de segurança utilizando o token explícito de EOF
    return {TokenType::Type::EOF_TOKEN, "EOF_TOKEN", "EOF", -1, -1};
}

Token peek_next() {
    if (current_token_index + 1 < global_tokens.size()) {
        return global_tokens[current_token_index + 1];
    }
    return {TokenType::Type::EOF_TOKEN, "EOF_TOKEN", "EOF", -1, -1};
}

void advance() {
    // FIX: Nunca avança além do EOF, prevenindo loops infinitos e acessos indevidos (-1/-1)
    if (current_token_index < global_tokens.size() && peek().type != TokenType::Type::EOF_TOKEN) {
        current_token_index++;
    }
}

void synchronize() {
    // FIX: Panic-mode aprimorado. Agora ele para NOS limites estruturais,
    // deixando que a função chamadora processe o limite de forma segura.
    while (true) {
        Token t = peek();

        if (t.type == TokenType::Type::EOF_TOKEN) break;

        // FIX: Paramos no ponto-e-vírgula mas NÃO o consumimos.
        // Assim, o match(SEMICOLON) que está pendente no topo da pilha
        // consome o token e evita um segundo erro falso em cascata.
        if (t.type == TokenType::Type::SEMICOLON) break;

        if (t.type == TokenType::Type::R_BRACE ||
            t.type == TokenType::Type::KW_IF ||
            t.type == TokenType::Type::KW_WHILE ||
            t.type == TokenType::Type::KW_FOR ||
            t.type == TokenType::Type::KW_RETURN ||
            t.type == TokenType::Type::KW_BREAK ||
            t.type == TokenType::Type::KW_CONTINUE ||
            is_data_type(t.type)) {
            break; // Para imediatamente antes da palavra-chave delimitadora
        }

        advance(); // Ignora o token problemático atual e continua procurando
    }
}

void match(TokenType::Type expected) {
    if (peek().type == expected) {
        if (syntactic_analyser_verbosity) {
            std::cout << "    [MATCH] " << token_type_to_string(expected) << " ('" << peek().text << "')\n";
        }
        advance();
    } else {
        std::cerr << "Erro Sintatico: Esperado [" << token_type_to_string(expected)
                  << "] mas encontrado [" << token_type_to_string(peek().type)
                  << "] ('" << peek().text << "') na linha " << peek().line
                  << ", coluna " << peek().column << std::endl;
        syntax_error_found = true;
        synchronize(); // Aciona o modo de recuperação (panic mode)
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

bool is_assign_op(TokenType::Type type) {
    return (type == TokenType::Type::OP_ASSIGN || type == TokenType::Type::OP_PLUS_ASSIGN ||
            type == TokenType::Type::OP_MINUS_ASSIGN || type == TokenType::Type::OP_MULT_ASSIGN ||
            type == TokenType::Type::OP_DIV_ASSIGN || type == TokenType::Type::OP_MOD_ASSIGN);
}

// ==========================================
// ESTRUTURA GLOBAL
// ==========================================

void parse_Program() {
    if (syntactic_analyser_verbosity) std::cout << "> parse_Program\n";
    while (true) {
        Token t = peek();
        if (t.type == TokenType::Type::EOF_TOKEN) break;

        if (is_data_type(t.type)) {
            parse_GlobalDecl();
        } else {
            std::cerr << "Erro Sintatico: Esperado um tipo no escopo global, mas encontrado ["
                      << token_type_to_string(t.type) << "] ('" << t.text << "') na linha " << t.line << std::endl;
            syntax_error_found = true;
            // FIX: Avança forçosamente para evitar loop infinito caso o token intruso seja uma '}' ou algo
            // que o synchronize() ignoraria.
            advance();
            synchronize();
        }
    }
}

void parse_GlobalDecl() {
    if (syntactic_analyser_verbosity) std::cout << "  > parse_GlobalDecl\n";
    advance(); // Consome o Tipo (is_data_type)

    if (peek().type == TokenType::Type::IDENTIFIER || peek().type == TokenType::Type::KW_MAIN) {
        if (syntactic_analyser_verbosity) std::cout << "    [MATCH] ID/main ('" << peek().text << "')\n";
        advance();
    } else {
        std::cerr << "Erro Sintatico: Esperado IDENTIFIER, mas encontrado ["
                  << token_type_to_string(peek().type) << "] na linha " << peek().line << std::endl;
        syntax_error_found = true;
        synchronize();
        return;
    }

    if (peek().type == TokenType::Type::L_PAREN) {
        parse_FuncDeclTail();
    } else {
        parse_VarDeclTail();
    }
}

void parse_VarDeclTail() {
    if (syntactic_analyser_verbosity) std::cout << "    > parse_VarDeclTail\n";
    if (peek().type == TokenType::Type::OP_ASSIGN) {
        match(TokenType::Type::OP_ASSIGN);
        parse_Expr();
        match(TokenType::Type::SEMICOLON);
    } else if (peek().type == TokenType::Type::L_BRACKET) {
        match(TokenType::Type::L_BRACKET);
        match(TokenType::Type::NUMBER_LITERAL);
        match(TokenType::Type::R_BRACKET);
        match(TokenType::Type::SEMICOLON);
    } else {
        match(TokenType::Type::SEMICOLON);
    }
}

void parse_FuncDeclTail() {
    if (syntactic_analyser_verbosity) std::cout << "    > parse_FuncDeclTail\n";
    match(TokenType::Type::L_PAREN);
    parse_Params();
    match(TokenType::Type::R_PAREN);
    parse_Block();
}

// ==========================================
// PARÂMETROS E BLOCOS
// ==========================================

void parse_Params() {
    if (syntactic_analyser_verbosity) std::cout << "      > parse_Params\n";
    if (is_data_type(peek().type)) {
        parse_Param();
        parse_ParamsTail();
    }
}

void parse_Param() {
    if (is_data_type(peek().type)) advance();
    else match(TokenType::Type::TYPE_INT);

    match(TokenType::Type::IDENTIFIER);
}

void parse_ParamsTail() {
    if (peek().type == TokenType::Type::COMMA) {
        match(TokenType::Type::COMMA);
        parse_Param();
        parse_ParamsTail();
    }
}

void parse_Block() {
    if (syntactic_analyser_verbosity) std::cout << "  > parse_Block\n";
    match(TokenType::Type::L_BRACE);
    parse_Statements();
    match(TokenType::Type::R_BRACE);
}

// ==========================================
// STATEMENTS
// ==========================================

void parse_Statements() {
    if (syntactic_analyser_verbosity) std::cout << "    > parse_Statements\n";
    // Parar de parsear statements apenas com delimitadores explícitos ou fim de arquivo
    while (peek().type != TokenType::Type::R_BRACE &&
           peek().type != TokenType::Type::EOF_TOKEN) {
        parse_Statement();
    }
}

void parse_Statement() {
    TokenType::Type pt = peek().type;

    if (is_data_type(pt)) parse_VarDeclStmt();
    else if (pt == TokenType::Type::KW_IF) parse_IfStmt();
    else if (pt == TokenType::Type::KW_WHILE) parse_WhileStmt();
    else if (pt == TokenType::Type::KW_FOR) parse_ForStmt();
    else if (pt == TokenType::Type::KW_RETURN) parse_ReturnStmt();
    else if (pt == TokenType::Type::KW_BREAK) parse_BreakStmt();
    else if (pt == TokenType::Type::KW_CONTINUE) parse_ContinueStmt();
    else if (pt == TokenType::Type::L_BRACE) parse_Block();
    else parse_ExprStmt();
}

void parse_VarDeclStmt() {
    if (syntactic_analyser_verbosity) std::cout << "    > parse_VarDeclStmt\n";
    advance(); // Tipo
    match(TokenType::Type::IDENTIFIER);
    parse_VarDeclTail();
}

void parse_IfStmt() {
    if (syntactic_analyser_verbosity) std::cout << "    > parse_IfStmt\n";
    match(TokenType::Type::KW_IF);
    match(TokenType::Type::L_PAREN);
    parse_Expr();
    match(TokenType::Type::R_PAREN);
    parse_Statement();

    if (peek().type == TokenType::Type::KW_ELSE) {
        if (syntactic_analyser_verbosity) std::cout << "    > parse_ElsePart\n";
        match(TokenType::Type::KW_ELSE);
        parse_Statement();
    }
}

void parse_WhileStmt() {
    if (syntactic_analyser_verbosity) std::cout << "    > parse_WhileStmt\n";
    match(TokenType::Type::KW_WHILE);
    match(TokenType::Type::L_PAREN);
    parse_Expr();
    match(TokenType::Type::R_PAREN);
    parse_Statement();
}

void parse_ForStmt() {
    if (syntactic_analyser_verbosity) std::cout << "    > parse_ForStmt\n";
    match(TokenType::Type::KW_FOR);
    match(TokenType::Type::L_PAREN);
    parse_ForInit();
    parse_Expr();
    match(TokenType::Type::SEMICOLON);
    parse_ForUpdate();
    match(TokenType::Type::R_PAREN);
    parse_Statement();
}

void parse_ForInit() {
    if (is_data_type(peek().type)) parse_VarDeclStmt();
    else if (peek().type == TokenType::Type::SEMICOLON) match(TokenType::Type::SEMICOLON);
    else parse_ExprStmt();
}

void parse_ForUpdate() {
    if (peek().type != TokenType::Type::R_PAREN) {
        parse_Expr();
    }
}

void parse_ReturnStmt() {
    if (syntactic_analyser_verbosity) std::cout << "    > parse_ReturnStmt\n";
    match(TokenType::Type::KW_RETURN);
    parse_ReturnTail();
}

void parse_ReturnTail() {
    if (peek().type == TokenType::Type::SEMICOLON) {
        match(TokenType::Type::SEMICOLON);
    } else {
        parse_Expr();
        match(TokenType::Type::SEMICOLON);
    }
}

void parse_BreakStmt() {
    match(TokenType::Type::KW_BREAK);
    match(TokenType::Type::SEMICOLON);
}

void parse_ContinueStmt() {
    match(TokenType::Type::KW_CONTINUE);
    match(TokenType::Type::SEMICOLON);
}

void parse_ExprStmt() {
    if (peek().type == TokenType::Type::SEMICOLON) {
        match(TokenType::Type::SEMICOLON);
    } else {
        parse_Expr();
        match(TokenType::Type::SEMICOLON);
    }
}

// ==========================================
// EXPRESSÕES (LL1 MAPPED - RIGHT ASSOCIATIVE)
// ==========================================

void parse_Expr() {
    parse_LogicalOrExpr();
    parse_AssignTail();
}

void parse_AssignTail() {
    if (is_assign_op(peek().type)) {
        // Correção de Bug do Parseador: Garante a passagem pelo Verbose e a validação correta.
        match(peek().type);
        parse_Expr();
    }
}

void parse_LogicalOrExpr() {
    parse_LogicalAndExpr();
    parse_LogicalOrTail();
}

void parse_LogicalOrTail() {
    if (peek().type == TokenType::Type::OP_OR) {
        match(TokenType::Type::OP_OR);
        parse_LogicalAndExpr();
        parse_LogicalOrTail();
    }
}

void parse_LogicalAndExpr() {
    parse_EqExpr();
    parse_LogicalAndTail();
}

void parse_LogicalAndTail() {
    if (peek().type == TokenType::Type::OP_AND) {
        match(TokenType::Type::OP_AND);
        parse_EqExpr();
        parse_LogicalAndTail();
    }
}

void parse_EqExpr() {
    parse_RelExpr();
    parse_EqTail();
}

void parse_EqTail() {
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_EQ || pt == TokenType::Type::OP_NE) {
        match(pt); // == ou !=
        parse_RelExpr();
        parse_EqTail();
    }
}

void parse_RelExpr() {
    parse_AddExpr();
    parse_RelTail();
}

void parse_RelTail() {
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_LT || pt == TokenType::Type::OP_GT ||
        pt == TokenType::Type::OP_LE || pt == TokenType::Type::OP_GE) {
        match(pt); // <, >, <=, >=
        parse_AddExpr();
        parse_RelTail();
    }
}

void parse_AddExpr() {
    parse_MultExpr();
    parse_AddTail();
}

void parse_AddTail() {
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_PLUS || pt == TokenType::Type::OP_MINUS) {
        match(pt); // + ou -
        parse_MultExpr();
        parse_AddTail();
    }
}

void parse_MultExpr() {
    parse_UnaryExpr();
    parse_MultTail();
}

void parse_MultTail() {
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_MULT || pt == TokenType::Type::OP_DIV || pt == TokenType::Type::OP_MOD) {
        match(pt); // *, /, %
        parse_UnaryExpr();
        parse_MultTail();
    }
}

void parse_UnaryExpr() {
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::OP_MINUS || pt == TokenType::Type::OP_NOT ||
        pt == TokenType::Type::OP_INC || pt == TokenType::Type::OP_DEC) {
        match(pt); // -, !, ++, --
        parse_UnaryExpr();
    } else {
        parse_PrimaryExpr();
        parse_PostfixTail();
    }
}

void parse_PrimaryExpr() {
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::IDENTIFIER) {
        match(TokenType::Type::IDENTIFIER);
    } else if (pt == TokenType::Type::NUMBER_LITERAL) {
        match(TokenType::Type::NUMBER_LITERAL);
    } else if (pt == TokenType::Type::STRING_LITERAL) {
        match(TokenType::Type::STRING_LITERAL);
    } else if (pt == TokenType::Type::L_PAREN) {
        match(TokenType::Type::L_PAREN);
        parse_Expr();
        match(TokenType::Type::R_PAREN);
    } else {
        std::cerr << "Erro Sintatico: Esperado Expressao, mas encontrado ["
                  << token_type_to_string(pt) << "] ('" << peek().text << "') na linha "
                  << peek().line << std::endl;
        syntax_error_found = true;

        // FIX: Consome o token inválido para evitar loop infinito de checagens da mesma expressão.
        // Contudo, poupa delimitadores estruturais garantindo que o fecho da expressão aconteça
        // corretamente nos pais (ex: a falta de operando não deve deletar um ';' necessário).
        if (pt != TokenType::Type::SEMICOLON &&
            pt != TokenType::Type::R_BRACE &&
            pt != TokenType::Type::EOF_TOKEN &&
            pt != TokenType::Type::R_PAREN &&
            pt != TokenType::Type::R_BRACKET) {
            advance();
        }
        synchronize();
    }
}

void parse_PostfixTail() {
    TokenType::Type pt = peek().type;
    if (pt == TokenType::Type::L_PAREN) {
        match(TokenType::Type::L_PAREN);
        parse_Args();
        match(TokenType::Type::R_PAREN);
        parse_PostfixTail();
    } else if (pt == TokenType::Type::L_BRACKET) {
        match(TokenType::Type::L_BRACKET);
        parse_Expr();
        match(TokenType::Type::R_BRACKET);
        parse_PostfixTail();
    } else if (pt == TokenType::Type::OP_INC || pt == TokenType::Type::OP_DEC) {
        match(pt); // ++, -- (sufixo)
        parse_PostfixTail();
    }
}

void parse_Args() {
    if (peek().type != TokenType::Type::R_PAREN) {
        parse_Expr();
        parse_ArgsTail();
    }
}

void parse_ArgsTail() {
    if (peek().type == TokenType::Type::COMMA) {
        match(TokenType::Type::COMMA);
        parse_Expr();
        parse_ArgsTail();
    }
}

// ==========================================
// PONTO DE ENTRADA DO ANALISADOR SINTÁTICO
// ==========================================

syntax_tree syntactic_analyser(std::vector<Token>& tokens, bool verbose) {
    global_tokens = tokens;
    current_token_index = 0;
    syntax_error_found = false;
    syntactic_analyser_verbosity = verbose;

    if (syntactic_analyser_verbosity) {
        std::cout << "\n=== INICIANDO ANALISE SINTATICA (TOP-DOWN) ===\n";
    }

    parse_Program();

    if (syntax_error_found) {
        std::cerr << "\n>>> Falha: A analise sintatica encontrou erros.\n";
    } else {
        std::cout << "\n>>> Sucesso: Analise sintatica concluida sem erros!\n";
    }

    return s_tree; // Omitido a construção de árvore por enquanto
}