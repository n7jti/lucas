#include "lucas/engine/engine.hpp"

#include "lucas/backend/evaluator.hpp"
#include "lucas/frontend/ast_renderer.hpp"
#include "lucas/frontend/parser.hpp"

#include <string>

namespace lucas::engine {
namespace {

Result session_error(std::string message) {
    return {std::nullopt, {{frontend::DiagnosticStage::Session, std::move(message), {}}}};
}

bool valid_slot(std::size_t slot) {
    return slot < 100;
}

} // namespace

Result calculate(std::string_view source) {
    return calculate(source, {});
}

Result calculate(std::string_view source, const CalculationContext& context) {
    frontend::ParseResult parsed = frontend::parse_expression(source);
    if (!parsed.success()) {
        return {std::nullopt, std::move(parsed.diagnostics)};
    }

    backend::EvaluationResult evaluated = backend::evaluate(*parsed.expression, {context.ans});
    if (!evaluated.success()) {
        return {evaluated.value, std::move(evaluated.diagnostics)};
    }

    std::optional<std::string> ast_tree;
    if (context.include_ast_tree) {
        ast_tree = frontend::render_ascii_tree(*parsed.expression);
    }
    return {evaluated.value, std::move(evaluated.diagnostics), std::move(ast_tree)};
}

Result Session::calculate(std::string_view source, bool include_ast_tree) {
    Result result = engine::calculate(source, {ans_, include_ast_tree});
    if (result.success() && result.value) {
        ans_ = *result.value;
    }
    return result;
}

Result Session::store(std::size_t slot) {
    if (!valid_slot(slot)) {
        return session_error("memory slot must be between m0 and m99");
    }
    memory_[slot] = ans_;
    return {};
}

Result Session::recall(std::size_t slot) {
    if (!valid_slot(slot)) {
        return session_error("memory slot must be between m0 and m99");
    }
    if (!memory_[slot]) {
        return session_error("memory slot m" + std::to_string(slot) + " is unset");
    }
    ans_ = *memory_[slot];
    return {ans_, {}};
}

Result Session::clear_memory(std::size_t slot) {
    if (!valid_slot(slot)) {
        return session_error("memory slot must be between m0 and m99");
    }
    memory_[slot].reset();
    return {};
}

Result Session::push() {
    stack_.push_back(ans_);
    return {};
}

Result Session::pop() {
    if (stack_.empty()) {
        return session_error("stack is empty");
    }
    ans_ = stack_.back();
    stack_.pop_back();
    return {ans_, {}};
}

Result Session::peek() const {
    if (stack_.empty()) {
        return session_error("stack is empty");
    }
    return {stack_.back(), {}};
}

Result Session::clear_stack() {
    std::vector<double>().swap(stack_);
    return {};
}

Result Session::clear_ans() {
    ans_ = 0.0;
    return {};
}

double Session::last_result() const noexcept {
    return ans_;
}

} // namespace lucas::engine
