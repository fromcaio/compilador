#include <iostream>
#include <fstream> // Correção do erro: 'incomplete type std::ifstream'
#include <string>
#include <vector>
#include <filesystem> // Para automação de detecção de extensão
#include "../HeaderFiles/lexical_analyser.h"
#include "../HeaderFiles/syntactic_analyser.h"

// Estrutura para consolidar as opções do compilador
struct CompilerConfig {
    std::string input_path;
    std::string output_path = "output.json";
    bool verbose = false;
    bool lexical_only = false;
};

// --- MAIN ---

int main(int argc, char *argv[]) {
    CompilerConfig config;

    // 1. Processamento de Argumentos
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") config.verbose = true;
        else if (arg == "-l") config.lexical_only = true;
        else if (arg == "-o" && i + 1 < argc) config.output_path = argv[++i];
        else if (arg[0] != '-') config.input_path = arg;
    }

    if (config.input_path.empty()) {
        std::cerr << "Uso: fcc <entrada.c | entrada.json> [-o saida.json] [-v] [-l]\n";
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
        // Fluxo padrão para arquivos de código (.c, .fcc, etc)
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

        if (lexical_errors) {
            // Se houver erro lexico, o verbose mostra o detalhamento visual
            if (config.verbose) {
                default_output(tokens, code_lines, error_indices);
            } else {
                std::cerr << "Erro lexico detectado. Use -v para detalhes.\n";
            }
            return 1;
        }

        if (config.lexical_only) {
            save_tokens_to_json(tokens, config.output_path);
            return 0; // Finaliza aqui se a flag -l for usada
        }
    }

    // 3. Analisador Sintático (Próxima Fase)
    if (tokens.empty()) {
        std::cerr << "Erro: Nenhum token disponivel para analise sintatica.\n";
        return 1;
    }

    std::cout << ">>> Iniciando Analise Sintatica...\n";
    syntax_tree s_tree = syntactic_analyser(tokens, false);

    return 0;
}