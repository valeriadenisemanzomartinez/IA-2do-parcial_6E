#include "semantic_tableau/pipeline.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <utility>

namespace semantic_tableau {
namespace {

enum class TokenKind { Word, LParen, RParen, And, Or, Not, If, Then, Implies, OnlyIf, Iff };

struct Token {
    TokenKind kind;
    std::string value;
};

std::string normalize(std::string text) {
    const std::vector<std::pair<std::string, std::string>> accents = {
        {"á", "a"}, {"é", "e"}, {"í", "i"}, {"ó", "o"}, {"ú", "u"}, {"ü", "u"}, {"ñ", "n"},
        {"Á", "a"}, {"É", "e"}, {"Í", "i"}, {"Ó", "o"}, {"Ú", "u"}, {"Ü", "u"}, {"Ñ", "n"}};
    for (const auto& [from, to] : accents) {
        size_t position = 0;
        while ((position = text.find(from, position)) != std::string::npos) {
            text.replace(position, from.size(), to);
            position += to.size();
        }
    }
    for (char& character : text) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte < 128) character = static_cast<char>(std::tolower(byte));
    }
    return text;
}

bool is_word_byte(unsigned char value) {
    return value >= 128 || std::isalnum(value) || value == '_';
}

std::vector<Token> tokenize(const std::string& input) {
    const std::string text = normalize(input);
    std::vector<std::string> words;
    for (size_t index = 0; index < text.size();) {
        const unsigned char value = static_cast<unsigned char>(text[index]);
        if (text[index] == '(' || text[index] == ')') {
            words.emplace_back(1, text[index++]);
        } else if (is_word_byte(value)) {
            const size_t start = index++;
            while (index < text.size() && is_word_byte(static_cast<unsigned char>(text[index]))) ++index;
            words.push_back(text.substr(start, index - start));
        } else {
            ++index;
        }
    }

    const std::vector<std::pair<std::vector<std::string>, TokenKind>> connectors = {
        {{"si", "y", "solo", "si"}, TokenKind::Iff}, {{"equivale", "a"}, TokenKind::Iff},
        {{"solo", "si"}, TokenKind::OnlyIf}, {{"entonces"}, TokenKind::Then},
        {{"implica"}, TokenKind::Implies}, {{"y"}, TokenKind::And}, {{"o"}, TokenKind::Or},
        {{"no"}, TokenKind::Not}, {{"si"}, TokenKind::If}};
    std::vector<Token> tokens;
    for (size_t index = 0; index < words.size();) {
        if (words[index] == "(" || words[index] == ")") {
            tokens.push_back({words[index] == "(" ? TokenKind::LParen : TokenKind::RParen, words[index]});
            ++index;
            continue;
        }
        bool matched = false;
        for (const auto& [phrase, kind] : connectors) {
            if (index + phrase.size() > words.size()) continue;
            if (!std::equal(phrase.begin(), phrase.end(), words.begin() + static_cast<std::ptrdiff_t>(index))) continue;
            tokens.push_back({kind, words[index]});
            index += phrase.size();
            matched = true;
            break;
        }
        if (!matched) tokens.push_back({TokenKind::Word, words[index++]});
    }
    if (tokens.empty()) throw AnalysisError("No se encontraron palabras para analizar.");
    return tokens;
}

class Parser {
public:
    Parser(const std::vector<Token>& tokens, const std::vector<Atom>& atoms) : tokens_(tokens) {
        for (const auto& atom : atoms) symbols_[normalize(atom.text)] = atom.symbol;
    }

    FormulaPtr parse() {
        auto formula = parse_iff();
        if (position_ != tokens_.size()) throw AnalysisError("Hay un conector inesperado al final de la formula.");
        return formula;
    }

private:
    FormulaPtr parse_iff() {
        auto formula = parse_implication();
        while (accept(TokenKind::Iff)) formula = make_binary(FormulaKind::Iff, formula, parse_implication());
        return formula;
    }

    FormulaPtr parse_implication() {
        if (accept(TokenKind::If)) {
            auto antecedent = parse_or();
            expect(TokenKind::Then, "Falta 'entonces' en la regla condicional.");
            return make_binary(FormulaKind::Implies, antecedent, parse_implication());
        }
        auto formula = parse_or();
        if (accept(TokenKind::OnlyIf) || accept(TokenKind::Implies)) {
            return make_binary(FormulaKind::Implies, formula, parse_implication());
        }
        return formula;
    }

    FormulaPtr parse_or() {
        auto formula = parse_and();
        while (accept(TokenKind::Or)) formula = make_binary(FormulaKind::Or, formula, parse_and());
        return formula;
    }

    FormulaPtr parse_and() {
        auto formula = parse_unary();
        while (accept(TokenKind::And)) formula = make_binary(FormulaKind::And, formula, parse_unary());
        return formula;
    }

    FormulaPtr parse_unary() {
        if (accept(TokenKind::Not)) return make_not(parse_unary());
        if (accept(TokenKind::LParen)) {
            auto formula = parse_iff();
            expect(TokenKind::RParen, "Falta cerrar un parentesis.");
            return formula;
        }
        return make_atom(read_atom());
    }

    std::string read_atom() {
        std::string phrase;
        while (position_ < tokens_.size() && tokens_[position_].kind == TokenKind::Word) {
            if (!phrase.empty()) phrase += ' ';
            phrase += tokens_[position_++].value;
        }
        if (phrase.empty()) throw AnalysisError("Se esperaba una proposicion atomica.");
        const auto found = symbols_.find(normalize(phrase));
        if (found == symbols_.end()) throw AnalysisError("No se pudo asociar la proposicion '" + phrase + "' con un atomo.");
        return found->second;
    }

    bool accept(TokenKind kind) {
        if (position_ < tokens_.size() && tokens_[position_].kind == kind) {
            ++position_;
            return true;
        }
        return false;
    }

    void expect(TokenKind kind, const std::string& message) {
        if (!accept(kind)) throw AnalysisError(message);
    }

    const std::vector<Token>& tokens_;
    std::map<std::string, std::string> symbols_;
    size_t position_ = 0;
};

struct SignedFormula {
    FormulaPtr formula;
    bool positive;
};

using Assignment = std::map<std::string, bool>;
using Expanded = std::set<std::string>;

std::vector<std::vector<SignedFormula>> alternatives(const FormulaPtr& formula, bool positive) {
    const auto sign = [](FormulaPtr item, bool value = true) { return SignedFormula{std::move(item), value}; };
    switch (formula->kind) {
        case FormulaKind::And:
            if (positive) return {{sign(formula->left), sign(formula->right)}};
            return {{sign(formula->left, false)}, {sign(formula->right, false)}};
        case FormulaKind::Or:
            if (positive) return {{sign(formula->left)}, {sign(formula->right)}};
            return {{sign(formula->left, false), sign(formula->right, false)}};
        case FormulaKind::Implies:
            if (positive) return {{sign(formula->left, false)}, {sign(formula->right)}};
            return {{sign(formula->left), sign(formula->right, false)}};
        case FormulaKind::Iff:
            if (positive) return {{sign(formula->left), sign(formula->right)}, {sign(formula->left, false), sign(formula->right, false)}};
            return {{sign(formula->left), sign(formula->right, false)}, {sign(formula->left, false), sign(formula->right)}};
        default: throw AnalysisError("No se puede expandir esta formula.");
    }
}

TableauNode expand_tableau(std::vector<SignedFormula> pending, Assignment assignment, Expanded expanded,
                           const std::vector<std::string>& atom_symbols,
                           std::vector<std::map<std::string, bool>>& models) {
    if (pending.empty()) {
        TableauNode leaf;
        leaf.label = "Rama abierta";
        leaf.open = true;
        for (const auto& symbol : atom_symbols) leaf.model[symbol] = assignment.count(symbol) ? assignment.at(symbol) : false;
        if (std::find(models.begin(), models.end(), leaf.model) == models.end()) models.push_back(leaf.model);
        return leaf;
    }

    SignedFormula current = pending.front();
    pending.erase(pending.begin());
    const std::string label = std::string(current.positive ? "T: " : "F: ") + format_formula(current.formula);
    TableauNode node;
    node.label = label;

    if (current.formula->kind == FormulaKind::Not) {
        pending.insert(pending.begin(), {current.formula->left, !current.positive});
        node.children.push_back(expand_tableau(std::move(pending), std::move(assignment), std::move(expanded), atom_symbols, models));
        return node;
    }
    if (current.formula->kind == FormulaKind::Atom) {
        const auto existing = assignment.find(current.formula->atom);
        if (existing != assignment.end() && existing->second != current.positive) {
            node.label += " [cierre]";
            node.closed = true;
            return node;
        }
        assignment[current.formula->atom] = current.positive;
        node.children.push_back(expand_tableau(std::move(pending), std::move(assignment), std::move(expanded), atom_symbols, models));
        return node;
    }

    const std::string key = formula_key(current.formula) + (current.positive ? "+" : "-");
    if (expanded.count(key)) {
        return expand_tableau(std::move(pending), std::move(assignment), std::move(expanded), atom_symbols, models);
    }
    expanded.insert(key);
    for (auto& branch : alternatives(current.formula, current.positive)) {
        auto branch_pending = branch;
        branch_pending.insert(branch_pending.end(), pending.begin(), pending.end());
        node.children.push_back(expand_tableau(std::move(branch_pending), assignment, expanded, atom_symbols, models));
    }
    return node;
}

void append_tree(const TableauNode& node, const std::string& prefix, bool last, bool root, std::vector<std::string>& lines) {
    if (!root) lines.push_back(prefix + (last ? "`-- " : "|-- ") + node.label);
    const std::string child_prefix = root ? "" : prefix + (last ? "    " : "|   ");
    for (size_t index = 0; index < node.children.size(); ++index) {
        append_tree(node.children[index], child_prefix, index + 1 == node.children.size(), false, lines);
    }
}

}  // namespace

std::vector<std::string> split_premises(const std::string& text) {
    std::vector<std::string> premises;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        const auto first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos) continue;
        const auto last = line.find_last_not_of(" \t\r");
        premises.push_back(line.substr(first, last - first + 1));
    }
    if (premises.empty()) throw AnalysisError("Escribe al menos una premisa no vacia.");
    return premises;
}

std::vector<Atom> extract_atoms(const std::vector<std::string>& premises) {
    std::vector<std::string> texts;
    std::set<std::string> seen;
    const auto store = [&texts, &seen](std::string& words) {
        if (words.empty()) return;
        const std::string key = normalize(words);
        if (seen.insert(key).second) texts.push_back(words);
        words.clear();
    };
    for (const auto& premise : premises) {
        std::string current;
        for (const auto& token : tokenize(premise)) {
            if (token.kind == TokenKind::Word) {
                if (!current.empty()) current += ' ';
                current += token.value;
            } else {
                store(current);
            }
        }
        store(current);
    }
    std::vector<Atom> atoms;
    const std::string alphabet = "pqrstuvwxyzabcdefghijklmno";
    for (size_t index = 0; index < texts.size(); ++index) {
        const std::string symbol = index < alphabet.size() ? std::string(1, alphabet[index]) : "p" + std::to_string(index + 1);
        atoms.push_back({symbol, texts[index]});
    }
    return atoms;
}

FormulaPtr translate(const std::string& text, const std::vector<Atom>& atoms) {
    return Parser(tokenize(text), atoms).parse();
}

AnalysisResult analyze(
    const std::string& text,
    const std::vector<std::pair<std::string, std::string>>& incompatibilities) {
    AnalysisResult result;
    result.premises = split_premises(text);
    result.atoms = extract_atoms(result.premises);
    for (const auto& premise : result.premises) result.formulas.push_back(translate(premise, result.atoms));

    for (const auto& [first_text, second_text] : incompatibilities) {
        const auto find_atom = [&result](const std::string& text) -> const Atom* {
            const std::string key = normalize(text);
            for (const auto& atom : result.atoms) {
                if (normalize(atom.text) == key) return &atom;
            }
            return nullptr;
        };
        const Atom* first = find_atom(first_text);
        const Atom* second = find_atom(second_text);
        if (first == nullptr || second == nullptr) {
            throw AnalysisError("Cada incompatibilidad debe coincidir con dos proposiciones atomicas de las premisas.");
        }
        if (first->symbol == second->symbol) {
            throw AnalysisError("Una proposicion no puede declararse incompatible consigo misma.");
        }
        result.contextual_constraints.push_back(
            make_not(make_binary(FormulaKind::And, make_atom(first->symbol), make_atom(second->symbol))));
        result.applied_incompatibilities.emplace_back(first->text, second->text);
    }

    std::vector<SignedFormula> pending;
    std::vector<std::string> symbols;
    for (const auto& atom : result.atoms) symbols.push_back(atom.symbol);
    for (const auto& formula : result.formulas) pending.push_back({formula, true});
    for (const auto& constraint : result.contextual_constraints) pending.push_back({constraint, true});
    result.root.label = "Premisas";
    result.root.children.push_back(expand_tableau(std::move(pending), {}, {}, symbols, result.models));
    return result;
}

std::string render_tree(const TableauNode& root) {
    std::vector<std::string> lines{root.label};
    for (size_t index = 0; index < root.children.size(); ++index) {
        append_tree(root.children[index], "", index + 1 == root.children.size(), false, lines);
    }
    std::ostringstream output;
    for (size_t index = 0; index < lines.size(); ++index) output << (index ? "\n" : "") << lines[index];
    return output.str();
}

std::string render_report(const AnalysisResult& result) {
    std::ostringstream output;
    output << "Escenario: " << (result.satisfiable() ? "true (satisfacible)" : "false (insatisfacible/contradictorio)")
           << "\n\nFormulas:\n";
    for (size_t index = 0; index < result.formulas.size(); ++index) {
        output << "  " << index + 1 << ". " << format_formula(result.formulas[index]) << '\n';
    }
    if (!result.contextual_constraints.empty()) {
        output << "Incompatibilidades aplicadas:\n";
        for (const auto& [first, second] : result.applied_incompatibilities) {
            output << "  '" << first << "' y '" << second << "' no pueden ser verdaderas a la vez.\n";
        }
    }

    if (result.satisfiable()) {
        output << "\nModelos validos (" << result.models.size() << " rama(s) abierta(s)):\n";
        for (size_t index = 0; index < result.models.size(); ++index) {
            output << "  M" << index + 1 << ": ";
            for (size_t atom_index = 0; atom_index < result.atoms.size(); ++atom_index) {
                if (atom_index) output << ", ";
                const auto& atom = result.atoms[atom_index];
                output << atom.symbol << '=' << (result.models[index].at(atom.symbol) ? "true" : "false") << " (" << atom.text << ')';
            }
            output << '\n';
        }
    } else {
        output << "\nNo hay ramas abiertas; todas las ramas se cerraron.\n";
    }
    return output.str();
}

}  // namespace semantic_tableau