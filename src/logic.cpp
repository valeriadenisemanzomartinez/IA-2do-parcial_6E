#include "semantic_tableau/logic.hpp"

#include <sstream>
#include <stdexcept>

namespace semantic_tableau {

FormulaPtr make_atom(const std::string& symbol) {
    return std::make_shared<Formula>(Formula{FormulaKind::Atom, symbol, nullptr, nullptr});
}

FormulaPtr make_not(FormulaPtr operand) {
    return std::make_shared<Formula>(Formula{FormulaKind::Not, "", std::move(operand), nullptr});
}

FormulaPtr make_binary(FormulaKind kind, FormulaPtr left, FormulaPtr right) {
    return std::make_shared<Formula>(Formula{kind, "", std::move(left), std::move(right)});
}

std::string format_formula(const FormulaPtr& formula) {
    if (!formula) throw std::invalid_argument("Formula nula.");
    if (formula->kind == FormulaKind::Atom) return formula->atom;
    if (formula->kind == FormulaKind::Not) {
        const std::string operand = format_formula(formula->left);
        return formula->left->kind == FormulaKind::Atom ? "NOT " + operand : "NOT (" + operand + ")";
    }

    std::string op;
    switch (formula->kind) {
        case FormulaKind::And: op = "AND"; break;
        case FormulaKind::Or: op = "OR"; break;
        case FormulaKind::Implies: op = "->"; break;
        case FormulaKind::Iff: op = "<->"; break;
        default: throw std::invalid_argument("Operador binario invalido.");
    }
    return "(" + format_formula(formula->left) + " " + op + " " + format_formula(formula->right) + ")";
}

std::string formula_key(const FormulaPtr& formula) {
    if (formula->kind == FormulaKind::Atom) return "A" + formula->atom;
    if (formula->kind == FormulaKind::Not) return "N(" + formula_key(formula->left) + ")";
    const char* tag = "";
    switch (formula->kind) {
        case FormulaKind::And: tag = "&"; break;
        case FormulaKind::Or: tag = "|"; break;
        case FormulaKind::Implies: tag = ">"; break;
        case FormulaKind::Iff: tag = "="; break;
        default: break;
    }
    return std::string(tag) + "(" + formula_key(formula->left) + "," + formula_key(formula->right) + ")";
}

bool evaluate_formula(const FormulaPtr& formula, const std::map<std::string, bool>& interpretation) {
    switch (formula->kind) {
        case FormulaKind::Atom: {
            const auto value = interpretation.find(formula->atom);
            if (value == interpretation.end()) throw std::invalid_argument("Falta el valor del atomo " + formula->atom + ".");
            return value->second;
        }
        case FormulaKind::Not:
            return !evaluate_formula(formula->left, interpretation);
        case FormulaKind::And:
            return evaluate_formula(formula->left, interpretation) && evaluate_formula(formula->right, interpretation);
        case FormulaKind::Or:
            return evaluate_formula(formula->left, interpretation) || evaluate_formula(formula->right, interpretation);
        case FormulaKind::Implies:
            return !evaluate_formula(formula->left, interpretation) || evaluate_formula(formula->right, interpretation);
        case FormulaKind::Iff:
            return evaluate_formula(formula->left, interpretation) == evaluate_formula(formula->right, interpretation);
    }
    throw std::invalid_argument("Tipo de formula desconocido.");
}

}  // namespace semantic_tableau