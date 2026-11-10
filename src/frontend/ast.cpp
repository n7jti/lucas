#include "lucas/frontend/ast.hpp"

#include <utility>

namespace lucas::frontend {

std::unique_ptr<Expression> Expression::number_literal(double value, SourceSpan span) {
    auto expression = std::make_unique<Expression>();
    expression->kind = ExpressionKind::Number;
    expression->span = span;
    expression->number = value;
    return expression;
}

std::unique_ptr<Expression> Expression::unary(
    char operation,
    std::unique_ptr<Expression> operand,
    SourceSpan span) {
    auto expression = std::make_unique<Expression>();
    expression->kind = ExpressionKind::Unary;
    expression->span = span;
    expression->operation = operation;
    expression->left = std::move(operand);
    return expression;
}

std::unique_ptr<Expression> Expression::binary(
    char operation,
    std::unique_ptr<Expression> left,
    std::unique_ptr<Expression> right,
    SourceSpan span) {
    auto expression = std::make_unique<Expression>();
    expression->kind = ExpressionKind::Binary;
    expression->span = span;
    expression->operation = operation;
    expression->left = std::move(left);
    expression->right = std::move(right);
    return expression;
}

std::unique_ptr<Expression> Expression::identifier(std::string name, SourceSpan span) {
    auto expression = std::make_unique<Expression>();
    expression->kind = ExpressionKind::Identifier;
    expression->span = span;
    expression->name = std::move(name);
    return expression;
}

std::unique_ptr<Expression> Expression::function(
    std::string name,
    std::unique_ptr<Expression> argument,
    SourceSpan span) {
    auto expression = std::make_unique<Expression>();
    expression->kind = ExpressionKind::Function;
    expression->span = span;
    expression->name = std::move(name);
    expression->left = std::move(argument);
    return expression;
}

} // namespace lucas::frontend
