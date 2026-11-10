#pragma once

#include "lucas/frontend/ast.hpp"
#include "lucas/frontend/diagnostic.hpp"

#include <memory>
#include <vector>

namespace lucas::frontend {

struct ParserContext {
    std::unique_ptr<Expression> root;
    std::vector<Diagnostic> diagnostics;
};

struct LexerState {
    std::size_t line{1};
    std::size_t column{1};
};

} // namespace lucas::frontend
