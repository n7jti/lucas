#pragma once

#include <cstddef>
#include <string>

namespace lucas::frontend {

struct SourceSpan {
    std::size_t line{1};
    std::size_t column{1};
    std::size_t end_line{1};
    std::size_t end_column{1};
};

enum class DiagnosticStage {
    Parse,
    Validation,
    Evaluation,
    Session,
};

struct Diagnostic {
    DiagnosticStage stage;
    std::string message;
    SourceSpan span;
};

} // namespace lucas::frontend
