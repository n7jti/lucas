#include "lucas/backend/evaluator.hpp"

#include <cmath>
#include <limits>
#include <numbers>
#include <string>

namespace lucas::backend {
namespace {

using frontend::Diagnostic;
using frontend::DiagnosticStage;
using frontend::Expression;
using frontend::ExpressionKind;

Diagnostic diagnostic(const Expression& expression, DiagnosticStage stage, std::string message) {
    return {stage, std::move(message), expression.span};
}

bool is_supported_constant(const std::string& name) {
    return name == "pi" || name == "e" || name == "ans";
}

bool is_supported_function(const std::string& name) {
    return name == "sin" || name == "cos" || name == "tan" ||
           name == "asin" || name == "acos" || name == "atan" ||
           name == "exp" || name == "ln" || name == "log10" ||
           name == "sqrt" || name == "abs";
}

void validate_expression(const Expression& expression, std::vector<Diagnostic>& diagnostics) {
    switch (expression.kind) {
    case ExpressionKind::Number:
        if (!std::isfinite(expression.number)) {
            diagnostics.push_back(diagnostic(expression, DiagnosticStage::Validation, "number literal is not finite"));
        }
        break;
    case ExpressionKind::Unary:
        if ((expression.operation != '+' && expression.operation != '-') || !expression.left || expression.right) {
            diagnostics.push_back(diagnostic(expression, DiagnosticStage::Validation, "invalid unary expression"));
            return;
        }
        validate_expression(*expression.left, diagnostics);
        break;
    case ExpressionKind::Binary:
        if ((expression.operation != '+' && expression.operation != '-' &&
             expression.operation != '*' && expression.operation != '/' &&
             expression.operation != '^') ||
            !expression.left || !expression.right) {
            diagnostics.push_back(diagnostic(expression, DiagnosticStage::Validation, "invalid binary expression"));
            return;
        }
        validate_expression(*expression.left, diagnostics);
        validate_expression(*expression.right, diagnostics);
        break;
    case ExpressionKind::Identifier:
        if (!is_supported_constant(expression.name) || expression.left || expression.right) {
            diagnostics.push_back(
                diagnostic(expression, DiagnosticStage::Validation, "unknown constant '" + expression.name + "'"));
        }
        break;
    case ExpressionKind::Function:
        if (!is_supported_function(expression.name) || !expression.left || expression.right) {
            diagnostics.push_back(
                diagnostic(expression, DiagnosticStage::Validation, "unknown or malformed function '" + expression.name + "'"));
            return;
        }
        validate_expression(*expression.left, diagnostics);
        break;
    }
}

std::optional<double> evaluate_expression(
    const Expression& expression,
    const EvaluationContext& context,
    std::vector<Diagnostic>& diagnostics) {
    if (expression.kind == ExpressionKind::Number) {
        return expression.number;
    }
    if (expression.kind == ExpressionKind::Identifier) {
        if (expression.name == "pi") {
            return std::numbers::pi_v<double>;
        }
        if (expression.name == "e") {
            return std::numbers::e_v<double>;
        }
        return context.ans;
    }

    const std::optional<double> left = evaluate_expression(*expression.left, context, diagnostics);
    if (!left) {
        return std::nullopt;
    }

    double value{};
    if (expression.kind == ExpressionKind::Unary) {
        value = expression.operation == '-' ? -*left : *left;
    } else if (expression.kind == ExpressionKind::Binary) {
        const std::optional<double> right = evaluate_expression(*expression.right, context, diagnostics);
        if (!right) {
            return std::nullopt;
        }
        switch (expression.operation) {
        case '+': value = *left + *right; break;
        case '-': value = *left - *right; break;
        case '*': value = *left * *right; break;
        case '/':
            if (*right == 0.0) {
                diagnostics.push_back(diagnostic(expression, DiagnosticStage::Evaluation, "division by zero"));
                return std::nullopt;
            }
            value = *left / *right;
            break;
        case '^':
            if (*left == 0.0 && *right <= 0.0) {
                diagnostics.push_back(diagnostic(expression, DiagnosticStage::Evaluation, "zero cannot be raised to a non-positive power"));
                return std::nullopt;
            }
            if (*left < 0.0 && std::trunc(*right) != *right) {
                diagnostics.push_back(diagnostic(expression, DiagnosticStage::Evaluation, "a negative base requires an integer exponent"));
                return std::nullopt;
            }
            value = std::pow(*left, *right);
            break;
        default:
            return std::nullopt;
        }
    } else {
        if ((expression.name == "ln" || expression.name == "log10") && *left <= 0.0) {
            diagnostics.push_back(diagnostic(expression, DiagnosticStage::Evaluation, "logarithm argument must be positive"));
            return std::nullopt;
        }
        if (expression.name == "sqrt" && *left < 0.0) {
            diagnostics.push_back(diagnostic(expression, DiagnosticStage::Evaluation, "square root argument must be non-negative"));
            return std::nullopt;
        }
        if ((expression.name == "asin" || expression.name == "acos") &&
            (*left < -1.0 || *left > 1.0)) {
            diagnostics.push_back(diagnostic(expression, DiagnosticStage::Evaluation, "inverse trigonometric argument must be between -1 and 1"));
            return std::nullopt;
        }
        if (expression.name == "tan" &&
            std::abs(std::cos(*left)) < 32.0 * std::numeric_limits<double>::epsilon()) {
            diagnostics.push_back(diagnostic(expression, DiagnosticStage::Evaluation, "tangent is undefined for this argument"));
            return std::nullopt;
        }

        if (expression.name == "sin") value = std::sin(*left);
        else if (expression.name == "cos") value = std::cos(*left);
        else if (expression.name == "tan") value = std::tan(*left);
        else if (expression.name == "asin") value = std::asin(*left);
        else if (expression.name == "acos") value = std::acos(*left);
        else if (expression.name == "atan") value = std::atan(*left);
        else if (expression.name == "exp") value = std::exp(*left);
        else if (expression.name == "ln") value = std::log(*left);
        else if (expression.name == "log10") value = std::log10(*left);
        else if (expression.name == "sqrt") value = std::sqrt(*left);
        else value = std::abs(*left);
    }

    if (!std::isfinite(value)) {
        diagnostics.push_back(diagnostic(expression, DiagnosticStage::Evaluation, "result is not finite"));
        return std::nullopt;
    }
    return value;
}

} // namespace

std::vector<frontend::Diagnostic> validate(
    const frontend::Expression& expression,
    const EvaluationContext&) {
    std::vector<frontend::Diagnostic> diagnostics;
    validate_expression(expression, diagnostics);
    return diagnostics;
}

EvaluationResult evaluate(const frontend::Expression& expression, const EvaluationContext& context) {
    EvaluationResult result;
    result.diagnostics = validate(expression, context);
    if (!result.diagnostics.empty()) {
        return result;
    }
    result.value = evaluate_expression(expression, context, result.diagnostics);
    return result;
}

} // namespace lucas::backend
