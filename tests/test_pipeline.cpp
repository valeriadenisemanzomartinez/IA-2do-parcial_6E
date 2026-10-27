#include "semantic_tableau/pipeline.hpp"

#include <cassert>
#include <iostream>

using namespace semantic_tableau;

int main() {
    const auto contradiction = analyze("llueve\nno llueve");
    assert(!contradiction.satisfiable());
    assert(contradiction.models.empty());

    const auto conditional = analyze("Si llueve entonces la calle se moja\nllueve");
    assert(conditional.satisfiable());
    assert(conditional.atoms.size() == 2);
    assert(conditional.models.front().at("q"));

    const auto day_and_night = analyze("es de dia\nes de noche", {{"es de dia", "es de noche"}});
    assert(!day_and_night.satisfiable());
    assert(day_and_night.contextual_constraints.size() == 1);
    assert(render_report(day_and_night).find("'es de dia' y 'es de noche' no pueden ser verdaderas a la vez") != std::string::npos);

    const auto time_scenario = analyze(
        "si es de dia entonces debo trabajar\nes de noche y trabajo", {{"es de dia", "es de noche"}});
    assert(time_scenario.satisfiable());
    for (const auto& model : time_scenario.models) {
        assert(!(model.at("p") && model.at("r")));
    }

    const auto generic_incompatibility = analyze("sensor activado\nsensor desactivado", {
        {"sensor activado", "sensor desactivado"}});
    assert(!generic_incompatibility.satisfiable());

    const auto unconstrained_day_and_night = analyze("es de dia\nes de noche");
    assert(unconstrained_day_and_night.satisfiable());

    bool rejected_unknown_atom = false;
    try {
        analyze("llueve", {{"llueve", "nieva"}});
    } catch (const AnalysisError&) {
        rejected_unknown_atom = true;
    }
    assert(rejected_unknown_atom);

    const auto conjunction = analyze("no estudia y aprueba");
    assert(conjunction.atoms.size() == 2);
    assert(format_formula(conjunction.formulas.front()) == "(NOT p AND q)");

    bool rejected = false;
    try {
        analyze("si llueve");
    } catch (const AnalysisError&) {
        rejected = true;
    }
    assert(rejected);

    const auto biconditional = analyze("(llueve o nieva) si y solo si hace frio");
    assert(biconditional.atoms.size() == 3);
    assert(render_report(biconditional).find("<->") != std::string::npos);

    const auto closed = analyze("Si llueve entonces nieva\nllueve\nno nieva");
    assert(!closed.satisfiable());
    assert(render_tree(closed.root).find("[cierre]") != std::string::npos);

    const auto disjunction = analyze("llueve o nieva");
    assert(disjunction.satisfiable());
    assert(render_tree(disjunction.root).find("Rama abierta") != std::string::npos);
    const auto satisfiable_report = render_report(disjunction);
    const auto unsatisfiable_report = render_report(contradiction);
    assert(satisfiable_report.find("Escenario: true (satisfacible)") != std::string::npos);
    assert(unsatisfiable_report.find("Escenario: false (insatisfacible/contradictorio)") != std::string::npos);
    assert(satisfiable_report.find("Resultado:") == std::string::npos);

    std::cout << "Todas las pruebas pasaron.\n";
    return 0;
}