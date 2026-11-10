#pragma once

#include "lucas/frontend/ast.hpp"
#include "lucas/frontend/diagnostic.hpp"

#include <optional>
#include <vector>

namespace lucas::backend {

struct EvaluationContext {
    double ans{0.0};
};

struct EvaluationResult {
    std::optional<double> value;
    std::vector<frontend::Diagnostic> diagnostics;

    [[nodiscard]] bool success() const noexcept {
        return value.has_value() && diagnostics.empty();
    }
};

std::vector<frontend::Diagnostic> validate(
    const frontend::Expression& expression,
    const EvaluationContext& context = {});
EvaluationResult evaluate(
    const frontend::Expression& expression,
    const EvaluationContext& context = {});

} // namespace lucas::backend
