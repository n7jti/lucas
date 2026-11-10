#pragma once

#include "lucas/frontend/diagnostic.hpp"

#include <optional>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace lucas::engine {

struct Result {
    std::optional<double> value;
    std::vector<frontend::Diagnostic> diagnostics;
    std::optional<std::string> ast_tree;

    [[nodiscard]] bool success() const noexcept {
        return diagnostics.empty();
    }
};

struct CalculationContext {
    double ans{0.0};
    bool include_ast_tree{false};
};

Result calculate(std::string_view source);
Result calculate(std::string_view source, const CalculationContext& context);

class Session {
public:
    Result calculate(std::string_view source, bool include_ast_tree = false);
    Result store(std::size_t slot);
    Result recall(std::size_t slot);
    Result clear_memory(std::size_t slot);
    Result push();
    Result pop();
    Result peek() const;
    Result clear_stack();
    Result clear_ans();

    [[nodiscard]] double last_result() const noexcept;

private:
    std::array<std::optional<double>, 100> memory_{};
    std::vector<double> stack_;
    double ans_{0.0};
};

} // namespace lucas::engine
