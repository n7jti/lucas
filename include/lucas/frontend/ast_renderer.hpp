#pragma once

#include "lucas/frontend/ast.hpp"

#include <string>

namespace lucas::frontend {

std::string render_ascii_tree(const Expression& expression);

} // namespace lucas::frontend
