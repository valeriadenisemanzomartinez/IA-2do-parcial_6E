#pragma once

#include <memory>
#include <map>
#include <string>
#include <vector>

namespace semantic_tableau {

enum class FormulaKind { Atom, Not, And, Or, Implies, Iff };

struct Formula {
    FormulaKind kind;
    std::string atom;
    std::shared_ptr<Formula> left;
    std::shared_ptr<Formula> right;
};

using FormulaPtr = std::shared_ptr<Formula>;

FormulaPtr make_atom(const std::string& symbol);
FormulaPtr make_not(FormulaPtr operand);
FormulaPtr make_binary(FormulaKind kind, FormulaPtr left, FormulaPtr right);
std::string format_formula(const FormulaPtr& formula);
std::string formula_key(const FormulaPtr& formula);
bool evaluate_formula(const FormulaPtr& formula, const std::map<std::string, bool>& interpretation);

}  // namespace semantic_tableau