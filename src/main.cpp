#include "semantic_tableau/pipeline.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string text;
    bool show_tree = true;
    std::string input_file;
    std::vector<std::pair<std::string, std::string>> incompatibilities;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--sin-arbol") {
            show_tree = false;
        } else if (argument == "--archivo" && index + 1 < argc) {
            input_file = argv[++index];
        } else if (argument == "--incompatible" && index + 1 < argc) {
            const std::string pair = argv[++index];
            const auto separator = pair.find('|');
            if (separator == std::string::npos || pair.find('|', separator + 1) != std::string::npos) {
                std::cerr << "Error: --incompatible requiere el formato 'proposicion A|proposicion B'.\n";
                return 2;
            }
            const std::string first = trim(pair.substr(0, separator));
            const std::string second = trim(pair.substr(separator + 1));
            if (first.empty() || second.empty()) {
                std::cerr << "Error: ambas proposiciones de --incompatible deben tener texto.\n";
                return 2;
            }
            incompatibilities.emplace_back(first, second);
        } else if (argument == "--help" || argument == "-h") {
            std::cout << "Uso: tableau-es [premisas...] [--archivo archivo.txt] [--incompatible 'A|B'] [--sin-arbol]\n"
                         "Escribe una premisa por linea o separa premisas con ';'.\n"
                         "Repite --incompatible para declarar varios pares incompatibles.\n";
            return 0;
        } else {
            if (!text.empty()) text += ' ';
            text += argument;
        }
    }

    if (!input_file.empty()) {
        std::ifstream file(input_file);
        if (!file) {
            std::cerr << "Error: no se pudo abrir el archivo '" << input_file << "'.\n";
            return 2;
        }
        std::ostringstream contents;
        contents << file.rdbuf();
        text = contents.str();
    } else if (text.empty()) {
        std::ostringstream contents;
        contents << std::cin.rdbuf();
        text = contents.str();
    }

    for (char& character : text) if (character == ';') character = '\n';
    try {
        const auto result = semantic_tableau::analyze(text, incompatibilities);
        std::cout << "Atomos:\n";
        for (const auto& atom : result.atoms) std::cout << "  " << atom.symbol << " = " << atom.text << '\n';
        std::cout << '\n' << semantic_tableau::render_report(result);
        if (show_tree) std::cout << "\nArbol semantico:\n" << semantic_tableau::render_tree(result.root) << '\n';
    } catch (const semantic_tableau::AnalysisError& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 2;
    }
    return 0;
}