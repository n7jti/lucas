#pragma once

#include "lucas/frontend/diagnostic.hpp"

#include <memory>
#include <string>

namespace lucas::frontend {

enum class ExpressionKind {
    Number,
    Unary,
    Binary,
    Identifier,
    Function,
};

struct Expression {
    ExpressionKind kind;
    SourceSpan span;
    double number{};
    char operation{};
    std::string name;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    static std::unique_ptr<Expression> number_literal(double value, SourceSpan span);
    static std::unique_ptr<Expression> unary(char operation, std::unique_ptr<Expression> operand, SourceSpan span);
    static std::unique_ptr<Expression> binary(
        char operation,
        std::unique_ptr<Expression> left,
        std::unique_ptr<Expression> right,
        SourceSpan span);
    static std::unique_ptr<Expression> identifier(std::string name, SourceSpan span);
    static std::unique_ptr<Expression> function(
        std::string name,
        std::unique_ptr<Expression> argument,
        SourceSpan span);
};

} // namespace lucas::frontend
