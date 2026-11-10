#pragma once

#include "lucas/frontend/ast.hpp"
#include "lucas/frontend/diagnostic.hpp"

#include <memory>
#include <string_view>
#include <vector>

namespace lucas::frontend {

struct ParseResult {
    std::unique_ptr<Expression> expression;
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] bool success() const noexcept {
        return expression != nullptr && diagnostics.empty();
    }
};

ParseResult parse_expression(std::string_view source);

} // namespace lucas::frontend
