#include "lucas/frontend/ast_renderer.hpp"

#include <sstream>
#include <string_view>
#include <vector>

namespace lucas::frontend {
namespace {

std::string label(const Expression& expression) {
    switch (expression.kind) {
    case ExpressionKind::Number: {
        std::ostringstream stream;
        stream << "Number(" << expression.number << ')';
        return stream.str();
    }
    case ExpressionKind::Unary:
        return "Unary(" + std::string(1, expression.operation) + ')';
    case ExpressionKind::Binary:
        return "Binary(" + std::string(1, expression.operation) + ')';
    case ExpressionKind::Identifier:
        return "Identifier(" + expression.name + ')';
    case ExpressionKind::Function:
        return "Function(" + expression.name + ')';
    }
    return "Unknown";
}

void append_tree(
    std::ostringstream& stream,
    const Expression& expression,
    std::string_view prefix,
    bool is_last,
    bool is_root) {
    if (!is_root) {
        stream << prefix << (is_last ? "`- " : "+- ");
    }
    stream << label(expression) << '\n';

    std::vector<const Expression*> children;
    if (expression.left) {
        children.push_back(expression.left.get());
    }
    if (expression.right) {
        children.push_back(expression.right.get());
    }

    std::string child_prefix(prefix);
    if (!is_root) {
        child_prefix += is_last ? "   " : "|  ";
    }
    for (std::size_t index = 0; index < children.size(); ++index) {
        append_tree(
            stream,
            *children[index],
            child_prefix,
            index + 1 == children.size(),
            false);
    }
}

} // namespace

std::string render_ascii_tree(const Expression& expression) {
    std::ostringstream stream;
    append_tree(stream, expression, {}, true, true);
    return stream.str();
}

} // namespace lucas::frontend
