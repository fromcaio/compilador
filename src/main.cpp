#include <iostream>
#include <string>
// cctype tem as funções isdigit, isalpha, isspace, etc.
#include <cctype>
#include <vector>
#include <regex>
#include <algorithm>

// *** DEFININDO OPERADORES ***
// operadores aritmeticos (sobrecarga do operador de atribuição)
const std::vector <std::string> arithmetic_operators = {"+", "-", "*", "/", "%", "++", "--", "+=", "-=", "*=", "/=", "%="};
// operadores relacionais
const std::vector <std::string> comparison_operators = {"==", "!=", "<", ">", "<=", ">="};
// operadores lógicos
const std::vector <std::string> logical_operators = {"&&", "||", "!"};
// primeiro caractere de um operador de multiplos caracteres
const std::vector <char> operator_start_characters = {'+', '-', '*', '/', '%', '=', '!', '<', '>', '&', '|'};

// *** DEFININDO SEPARADORES DE TOKENS ***
// OBS: operadores também são separadores de tokens
// vamos definiir os sepadores de tokens como um conjunto de caracteres
const std::vector <std::string> token_separators = {" ", "\t", "\n", "(", ")", "{", "}", "[", "]", ";", ",", ":", "\"", "\'"};

// *** DEFININDO PALAVRAS-CHAVE ***
const std::vector <std::string> keywords = {"if", "else", "while", "for", "return", "int", "float", "string", "bool", "break", "continue", "complex", "void"};

// *** DEFININDO REGRA IDENTIFICADOR ***
const std::string identifier_pattern = "^[a-zA-Z_][a-zA-Z0-9_]*$";

// *** DEFININDO REGRA DE CONSTANTES LITERAIS ***
// *** Regra de string literal ***
const std::string string_literal_pattern = "^\"(\\\\.|[^\"\\\\])*\"$";
// \\\\ é utilizado para enviar um caractere de barra invertida seguindo de qualquer caractere, incluindo aspas, para permitir a inclusão de caracteres especiais dentro da string literal.
// ^serve para indicar o início da string quando está posicionado no inicio da expressao regular
// [^ ] significa qualquer caractere que não seja o que está dentro dos colchetes
// * pode ocorrer zero ou mais vezes
// *** Regra de número literal ***
const std::string number_literal_pattern = "^[0-9]+(\\.[0-9]+)?$";
// \\. significa um ponto literal, pois em expressões regulares o ponto é um caractere curinga que corresponde a qualquer caractere.
// + o padrão deve occorrer uma ou mais vezes
// ? o padrão pode ocorrer zero ou uma vez

struct Token {
    std::string type;
    std::string text;
    int line_number;
    int column_number;
};

int main() {

    std::vector <std::string> code;
    std::cout << "Insira o codigo a ser processado linha por linha " << std::endl;
    std::cout << "Insira uma linha com 'END' para finalizar a entrada de código." << std::endl;
    
    while (true) {
        std::string line;
        // ao receber texto com getline o caractere /n é removido e o caractere /0 é adicionado ao final da string
        std::getline(std::cin, line);
        if (line == "END") {
            break;
        }
        //push_back adiciona a linha ao final do vetor
        code.push_back(line + "\n");
    }

    std::vector <Token> tokens;

    std::vector <std::string> all_separators;
    all_separators.insert(all_separators.end(), token_separators.begin(), token_separators.end());
    all_separators.insert(all_separators.end(), arithmetic_operators.begin(), arithmetic_operators.end());
    all_separators.insert(all_separators.end(), comparison_operators.begin(), comparison_operators.end());
    all_separators.insert(all_separators.end(), logical_operators.begin(), logical_operators.end());

    std::vector <std::string> all_operators;
    all_operators.insert(all_operators.end(), arithmetic_operators.begin(), arithmetic_operators.end());
    all_operators.insert(all_operators.end(), comparison_operators.begin(), comparison_operators.end());
    all_operators.insert(all_operators.end(), logical_operators.begin(), logical_operators.end());

    // aqui estamos fazendo uma cópia da lista de operadores para a lista de operadores de multiplos caracteres, para depois remover os operadores de um caractere da lista de operadores de multiplos caracteres
    std::vector <std::string> all_multi_char_operators = all_operators;
    // removendo os operadores de um caractere da lista de operadores de multiplos caracteres
    all_multi_char_operators.erase(std::remove_if(all_multi_char_operators.begin(), all_multi_char_operators.end(), [](const std::string& op) {
        return op.size() == 1;
    }), all_multi_char_operators.end());
    
    for (int i = 0; i < code.size(); i++) {
        std::string line = code[i];
        int line_number = i + 1; // numero da linha começa com 1
        int regex_ready = 0; // flag para indicar se já encontramos 2 separadores, ai podemos enviar a string entre os dois separadores para a regex
        int token_start = 0;
        int token_end = 0;

        // *** TOKENIZAÇÃO ***
        // vamos percorrer cada caractere da linha e construir os tokens
        for (int j = 0; j < line.size();) {
            char c = line[j];

            if(std::isspace(c)) {
                if (regex_ready == 0) {
                    regex_ready = 1;
                    token_start = j + 1;
                }
                if (regex_ready == 1) {
                    regex_ready = 2;
                    token_end = j-1;
                }
                j++;
                std::cout << "Encontramos um espaço em branco, regex_ready: " << regex_ready << ", token_start: " << token_start << ", token_end: " << token_end << std::endl;
                continue;
            }

            // se regex_ready for 2, significa que encontramos dois separadores e podemos enviar a string entre os dois separadores para a regex
            if (regex_ready == 2) {
                // enviamos para a regex a string entre os dois separadores, que é o token que queremos identificar
                regex_ready = 0;
                std::cout << "Enviando para a regex: " << line.substr(token_start, token_end - token_start + 1) << std::endl;
            }

            // se o caractere for um separador ou operador (também são separadores)
            if (std::find(all_separators.begin(), all_separators.end(), std::string(1, c)) != all_separators.end()){
                // caso seja um caractere de um operador de multiplos caracteres, vamos verificar o proximo caracter para ver se forma um operador de multiplos caracteres
                // j + 1 < line.size() para garantir que não estamos acessando um indice fora do vetor
                if (std::find(operator_start_characters.begin(), operator_start_characters.end(), c) != operator_start_characters.end() && j + 1 < line.size()) {
                   // vamos olhar o proximo caractere e verificar se forma um operador de multiplos caracteres 
                   std::string potential_operator = std::string(1, c) + line[j + 1];
                   if (std::find(all_multi_char_operators.begin(), all_multi_char_operators.end(), potential_operator) != all_multi_char_operators.end()) {
                        // se formar um operador de multiplos caracteres, vamos adicionar o operador como um token e pular o proximo caractere
                        tokens.push_back({"multi_char_operator", potential_operator, line_number, j + 1});
                        if (regex_ready == 0) {
                            token_start = j + 2;
                        }
                        if (regex_ready == 1) {
                            token_end = j - 1;
                            regex_ready = 2;
                        }
                        // como consumimos dois caracteres precisamos pular 2 caracteres
                        j += 2;

                        std::cout << "Encontramos um operador de multiplos caracteres, regex_ready: " << regex_ready << ", token_start: " << token_start << ", token_end: " << token_end << std::endl;
                        continue;
                    }
                }
                else {
                    // se não formar um operador de multiplos caracteres, vamos adicionar o operador de um caractere como um token
                    tokens.push_back({"separator", std::string(1, c), line_number, j + 1});
                    if (regex_ready == 0) {
                        token_start = j + 1;
                    }
                    if (regex_ready == 1) {
                        token_end = j - 1;
                        regex_ready = 2;
                    }
                    j++;
                    std::cout << "Encontramos um separador" << c <<"regex_ready: " << regex_ready << ", token_start: " << token_start << ", token_end: " << token_end << std::endl;
                    continue;
                }
            }
            if (regex_ready == 0) {
                token_start = j;
                regex_ready = 1;
            }
            j++;
        }
    }

    return 0;
}