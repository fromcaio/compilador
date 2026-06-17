#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include "../HeaderFiles/syntactic_analyser.h"

struct CompilerConfig {
    std::string input_path;
    std::string output_path = "output.json";
    bool verbose_lexical = false;
    bool verbose_syntactic = false;
    bool lexical_only = false;
    bool show_symbols = false; // flag --sym
};

// --- HELP ---
void print_usage() {
    std::cout << "Uso: fcc <entrada.c | entrada.json> [opcoes]\n\n"
              << "Opcoes:\n"
              << "  -v, --verbose    Ativa a saida detalhada para TODAS as fases.\n"
              << "  --vlex           Ativa a saida detalhada APENAS para a analise lexica.\n"
              << "  --vsyn           Ativa a saida detalhada APENAS para a analise sintatica.\n"
              << "  -l               Executa apenas a analise lexica e salva os tokens no arquivo de saida.\n"
              << "  -s, --sym        Imprime a tabela de simbolos ao final da analise.\n"
              << "  -o <arquivo>     Especifica o arquivo de saida para os tokens (padrao: output.json).\n"
              << "  -h, --help       Mostra esta mensagem de ajuda.\n\n"
              << "Exemplos:\n"
              << "  fcc meu_programa.c             # Analise silenciosa\n"
              << "  fcc meu_programa.c -v          # Log completo de tokens e regras sintaticas\n"
              << "  fcc meu_programa.c --sym       # Exibe a tabela de simbolos ao final\n"
              << "  fcc meu_programa.c -v --sym    # Verbose + tabela de simbolos\n"
              << "  fcc meu_programa.c -l -o t.json# Apenas analise lexica\n";
}

// --- MAIN ---

int main(int argc, char *argv[]) {
    CompilerConfig config;

    // 1. Processamento de Argumentos
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            config.verbose_lexical = true;
            config.verbose_syntactic = true;
            config.show_symbols = true;
        }
        else if (arg == "--vlex")  config.verbose_lexical = true;
        else if (arg == "--vsyn")  config.verbose_syntactic = true;
        else if (arg == "--help" || arg == "-h") { print_usage(); return 0; }
        else if (arg == "-l")      config.lexical_only = true;
        else if (arg == "-s" || arg == "--sym") config.show_symbols = true;
        else if (arg == "-o" && i + 1 < argc)  config.output_path = argv[++i];
        else if (arg[0] != '-')    config.input_path = arg;
    }

    if (config.input_path.empty()) {
        std::cerr << "Uso: fcc <entrada.c | entrada.json> [-o saida.json] [-v | --vlex | --vsyn] [-l] [-s]\n";
        return 1;
    }

    std::vector<Token> tokens;
    std::filesystem::path p(config.input_path);
    std::string extension = p.extension().string();

    // 2. Automação por Extensão
    if (extension == ".json") {
        std::cout << ">>> Entrada detectada como JSON. Pulando para analise sintatica...\n";
        load_tokens_from_json(config.input_path, tokens);
    }
    else {
        std::ifstream file(config.input_path);
        if (!file.is_open()) {
            std::cerr << "Erro: Nao foi possivel abrir o arquivo: " << config.input_path << "\n";
            return 1;
        }

        std::vector<std::string> code_lines;
        std::string line;
        while (std::getline(file, line)) {
            code_lines.push_back(line + "\n");
        }
        file.close();

        std::vector<size_t> error_indices;
        run_lexical_analysis(code_lines, tokens, error_indices);

        if (config.verbose_lexical && !tokens.empty()) {
            verbose_output(tokens);
        }

        if (lexical_errors) {
            if (config.verbose_lexical) {
                default_output(tokens, code_lines, error_indices);
            } else {
                std::cerr << "Erro lexico detectado. Use --vlex ou -v para detalhes.\n";
            }
            return 1;
        }

        if (config.lexical_only) {
            save_tokens_to_json(tokens, config.output_path);
            return 0;
        }
    }

    // 3. Análise Sintática + Semântica
    if (tokens.empty()) {
        std::cerr << "Erro: Nenhum token disponivel para analise.\n";
        return 1;
    }

    bool syn_ok = syntactic_analyser(tokens, config.verbose_syntactic);

    // Relatório de erros/sucesso
    if (!syn_ok) {
        std::cerr << ">>> Falha: A analise sintatica encontrou erros.\n";
    }
    if (semantic_error_found) {
        std::cerr << ">>> Falha: A analise semantica encontrou erros.\n";
    }
    if (syn_ok && !semantic_error_found) {
        std::cout << ">>> Sucesso: Analise concluida sem erros!\n";
    }

    // 4. Tabela de Símbolos (opcional)
    if (config.show_symbols) {
        print_symbol_table();
    }

    return (syn_ok && !semantic_error_found) ? 0 : 1;
}
