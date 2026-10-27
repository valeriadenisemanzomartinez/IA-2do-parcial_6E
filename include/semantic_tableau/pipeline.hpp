#pragma once

#include "semantic_tableau/logic.hpp"

#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace semantic_tableau {

class AnalysisError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct Atom {
    std::string symbol;
    std::string text;
};

struct TableauNode {
    std::string label;
    std::vector<TableauNode> children;
    bool closed = false;
    bool open = false;
    std::map<std::string, bool> model;
};

struct AnalysisResult {
    std::vector<std::string> premises;
    std::vector<Atom> atoms;
    std::vector<FormulaPtr> formulas;
    std::vector<FormulaPtr> contextual_constraints;
    std::vector<std::pair<std::string, std::string>> applied_incompatibilities;
    TableauNode root;
    std::vector<std::map<std::string, bool>> models;

    bool satisfiable() const { return !models.empty(); }
};

std::vector<std::string> split_premises(const std::string& text);
std::vector<Atom> extract_atoms(const std::vector<std::string>& premises);
FormulaPtr translate(const std::string& text, const std::vector<Atom>& atoms);
AnalysisResult analyze(
    const std::string& text,
    const std::vector<std::pair<std::string, std::string>>& incompatibilities = {});
std::string render_tree(const TableauNode& root);
std::string render_report(const AnalysisResult& result);

}  // namespace semantic_tableau