#include "lucas/backend/evaluator.hpp"
#include "lucas/engine/engine.hpp"
#include "lucas/frontend/ast.hpp"
#include "lucas/frontend/ast_renderer.hpp"
#include "lucas/frontend/parser.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <string_view>

namespace {

int failures = 0;

void expect_result(std::string_view source, double expected) {
    const lucas::engine::Result result = lucas::engine::calculate(source);
    if (!result.success() || std::abs(*result.value - expected) > 1e-12) {
        std::cerr << "expected " << source << " to equal " << expected << '\n';
        ++failures;
    }
}

void expect_error(std::string_view source, std::string_view message) {
    const lucas::engine::Result result = lucas::engine::calculate(source);
    if (result.success() || result.diagnostics.empty() ||
        result.diagnostics.front().message.find(message) == std::string::npos) {
        std::cerr << "expected " << source << " to produce an error containing " << message << '\n';
        ++failures;
    }
}

void expect_tree(std::string_view source, std::string_view expected) {
    const lucas::frontend::ParseResult parsed = lucas::frontend::parse_expression(source);
    if (!parsed.success() || lucas::frontend::render_ascii_tree(*parsed.expression) != expected) {
        std::cerr << "unexpected tree for " << source << '\n';
        ++failures;
    }
}

} // namespace

int main() {
    expect_result("2 + 3 * 4", 14.0);
    expect_result("-(2 + 3) * .5", -2.5);
    expect_result("8 / 2 / 2", 2.0);
    expect_result("2 ^ 3 ^ 2", 512.0);
    expect_result("-2 ^ 2", -4.0);
    expect_result("2 ^ -2", 0.25);
    expect_result("sin(pi / 2) + cos(0)", 2.0);
    expect_result("sqrt(9) + abs(-3) + ln(e) + log10(100)", 9.0);
    expect_result("asin(1) + acos(1) + atan(1)", 3.14159265358979323846 / 2.0 + 3.14159265358979323846 / 4.0);
    expect_error("1 + )", "syntax error");
    expect_error("2 @ 3", "syntax error");
    expect_error("1 / 0", "division by zero");
    expect_error("tau", "unknown constant");
    expect_error("factorial(3)", "unknown or malformed function");
    expect_error("sqrt(-1)", "square root argument");
    expect_error("ln(0)", "logarithm argument");
    expect_error("acos(2)", "inverse trigonometric argument");
    expect_error("tan(pi / 2)", "tangent is undefined");
    expect_error("(-2) ^ .5", "negative base");
    expect_tree(
        "2 + sin(pi)",
        "Binary(+)\n"
        "+- Number(2)\n"
        "`- Function(sin)\n"
        "   `- Identifier(pi)\n");
    expect_tree("1.23456789", "Number(1.23457)\n");

    lucas::engine::Session session;
    expect_result("ans", 0.0);
    const auto first = session.calculate("2 + 3");
    if (!first.success() || !first.value || *first.value != 5.0) {
        std::cerr << "expected session calculation to succeed\n";
        ++failures;
    }
    const auto chained = session.calculate("ans * 4");
    if (!chained.success() || !chained.value || *chained.value != 20.0) {
        std::cerr << "expected ans to use the session last result\n";
        ++failures;
    }
    const auto failed_calculation = session.calculate("sqrt(-1)");
    if (failed_calculation.success() || session.last_result() != 20.0) {
        std::cerr << "expected failed calculation to preserve ans\n";
        ++failures;
    }
    if (!session.store(0).success() || !session.clear_ans().success() || session.last_result() != 0.0) {
        std::cerr << "expected store and clear ans to succeed\n";
        ++failures;
    }
    const auto recalled = session.recall(0);
    if (!recalled.success() || !recalled.value || *recalled.value != 20.0 || session.last_result() != 20.0) {
        std::cerr << "expected recall to restore ans\n";
        ++failures;
    }
    if (!session.clear_memory(0).success() || session.recall(0).success()) {
        std::cerr << "expected cleared memory slot to be unset\n";
        ++failures;
    }
    if (!session.push().success() || !session.clear_ans().success()) {
        std::cerr << "expected push and clear ans to succeed\n";
        ++failures;
    }
    const auto peeked = session.peek();
    const auto popped = session.pop();
    if (!peeked.success() || !peeked.value || *peeked.value != 20.0 ||
        !popped.success() || !popped.value || *popped.value != 20.0 ||
        session.last_result() != 20.0 || session.peek().success()) {
        std::cerr << "expected stack operations to transfer ans and reject empty peek\n";
        ++failures;
    }
    if (!session.push().success() || !session.clear_stack().success() || session.pop().success()) {
        std::cerr << "expected clear stack to empty the stack\n";
        ++failures;
    }
    const auto normal_result = session.calculate("2 + 3");
    const auto verbose_result = session.calculate("2 + 3", true);
    const auto verbose_error = session.calculate("sqrt(-1)", true);
    if (normal_result.ast_tree || !verbose_result.ast_tree ||
        *verbose_result.ast_tree != "Binary(+)\n+- Number(2)\n`- Number(3)\n" ||
        verbose_error.ast_tree) {
        std::cerr << "expected optional AST output only for successful verbose calculations\n";
        ++failures;
    }

    auto malformed = lucas::frontend::Expression::unary('*', nullptr, {});
    const auto diagnostics = lucas::backend::validate(*malformed);
    if (diagnostics.empty()) {
        std::cerr << "expected malformed AST validation to fail\n";
        ++failures;
    }

    return failures == 0 ? 0 : 1;
}
