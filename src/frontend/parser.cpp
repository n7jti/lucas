#include "lucas/frontend/parser.hpp"

#include "parser_context.hpp"
#include "parser.hpp"

#include <limits>
#include <string>

struct yy_buffer_state;
using YY_BUFFER_STATE = yy_buffer_state*;

int yylex_init_extra(lucas::frontend::LexerState* user_defined, yyscan_t* scanner);
YY_BUFFER_STATE yy_scan_bytes(const char* bytes, int length, yyscan_t scanner);
void yy_delete_buffer(YY_BUFFER_STATE buffer, yyscan_t scanner);
int yylex_destroy(yyscan_t scanner);

namespace lucas::frontend {

ParseResult parse_expression(std::string_view source) {
    ParseResult result;
    if (source.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        result.diagnostics.push_back(
            {DiagnosticStage::Parse, "input is too large to parse", {}});
        return result;
    }

    ParserContext context;
    LexerState lexer_state;
    yyscan_t scanner{};
    if (yylex_init_extra(&lexer_state, &scanner) != 0) {
        result.diagnostics.push_back(
            {DiagnosticStage::Parse, "could not initialize the lexer", {}});
        return result;
    }

    YY_BUFFER_STATE buffer = yy_scan_bytes(source.data(), static_cast<int>(source.size()), scanner);
    if (buffer == nullptr) {
        yylex_destroy(scanner);
        result.diagnostics.push_back(
            {DiagnosticStage::Parse, "could not prepare input for the lexer", {}});
        return result;
    }

    const int parse_status = yyparse(&context, scanner);
    yy_delete_buffer(buffer, scanner);
    yylex_destroy(scanner);

    result.expression = std::move(context.root);
    result.diagnostics = std::move(context.diagnostics);
    if (parse_status != 0 && result.diagnostics.empty()) {
        result.diagnostics.push_back(
            {DiagnosticStage::Parse, "parser could not process the input", {}});
    }
    return result;
}

} // namespace lucas::frontend
