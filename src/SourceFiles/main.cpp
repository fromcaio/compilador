#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include "../HeaderFiles/lexical_analyser.h"
#include "../HeaderFiles/syntactic_analyser.h"

// Estrutura para consolidar as opções do compilador
struct CompilerConfig {
    std::string input_path;
    std::string output_path = "output.json";
    bool verbose_lexical = false;
    bool verbose_syntactic = false;
    bool lexical_only = false;
};

// --- HELP ---
void print_usage() {
    std::cout << "Uso: fcc <entrada.c | entrada.json> [opcoes]\n\n"
              << "Opcoes:\n"
              << "  -v, --verbose    Ativa a saida detalhada para TODAS as fases (lexica e sintatica).\n"
              << "  --vlex           Ativa a saida detalhada APENAS para a analise lexica.\n"
              << "  --vsyn           Ativa a saida detalhada APENAS para a analise sintatica.\n"
              << "  -l               Executa apenas a analise lexica e salva os tokens no arquivo de saida.\n"
              << "  -o <arquivo>     Especifica o arquivo de saida para os tokens (padrao: output.json).\n"
              << "  -h, --help       Mostra esta mensagem de ajuda.\n\n"
              << "Exemplos:\n"
              << "  fcc meu_programa.c             # Realiza analise de forma silenciosa\n"
              << "  fcc meu_programa.c -v          # Analise com log completo de tokens e regras sintaticas\n"
              << "  fcc meu_programa.c --vsyn      # Exibe apenas os traces da arvore sintatica\n"
              << "  fcc meu_programa.c --vlex      # Exibe apenas a lista de tokens gerada\n"
              << "  fcc meu_programa.c -l -o t.json # Apenas analise lexica, salva em t.json\n";
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
        }
        else if (arg == "--vlex") config.verbose_lexical = true;
        else if (arg == "--vsyn") config.verbose_syntactic = true;
        else if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        }
        else if (arg == "-l") config.lexical_only = true;
        else if (arg == "-o" && i + 1 < argc) config.output_path = argv[++i];
        else if (arg[0] != '-') config.input_path = arg;
    }

    if (config.input_path.empty()) {
        std::cerr << "Uso: fcc <entrada.c | entrada.json> [-o saida.json] [-v | --vlex | --vsyn] [-l]\n";
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

        // Se a verbosidade léxica estiver ativa, mostramos o stream de tokens completo
        if (config.verbose_lexical && !tokens.empty()) {
            verbose_output(tokens);
        }

        if (lexical_errors) {
            // Mostramos o visualizador de erros bonitinho com ponteiros (^)
            // somente se houver verbosidade léxica ativada.
            if (config.verbose_lexical) {
                default_output(tokens, code_lines, error_indices);
            } else {
                std::cerr << "Erro lexico detectado. Use --vlex ou -v para detalhes.\n";
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
    syntax_tree s_tree = syntactic_analyser(tokens, config.verbose_syntactic);

    return 0;
}