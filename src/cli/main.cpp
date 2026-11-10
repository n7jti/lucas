#include "lucas/engine/engine.hpp"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string format_number(double value) {
    std::ostringstream stream;
    stream << std::defaultfloat << std::setprecision(15) << value;
    return stream.str();
}

void print_diagnostics(const std::vector<lucas::frontend::Diagnostic>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.stage == lucas::frontend::DiagnosticStage::Session) {
            std::cerr << "error: " << diagnostic.message << '\n';
        } else {
            std::cerr << "error at " << diagnostic.span.line << ':' << diagnostic.span.column
                      << ": " << diagnostic.message << '\n';
        }
    }
}

int print_result(const lucas::engine::Result& result) {
    if (!result.success()) {
        print_diagnostics(result.diagnostics);
        return 1;
    }
    if (result.ast_tree) {
        std::cout << *result.ast_tree;
    }
    if (result.value) {
        std::cout << format_number(*result.value) << '\n';
    }
    return 0;
}

int calculate_and_print(
    lucas::engine::Session& session,
    const std::string& input,
    bool include_ast_tree = false) {
    return print_result(session.calculate(input, include_ast_tree));
}

bool parse_memory_slot(const std::string& text, std::size_t& slot) {
    if (text.size() < 2 || text.size() > 3 || text.front() != 'm') {
        return false;
    }

    std::size_t value = 0;
    for (std::size_t index = 1; index < text.size(); ++index) {
        const unsigned char character = static_cast<unsigned char>(text[index]);
        if (!std::isdigit(character)) {
            return false;
        }
        value = value * 10 + static_cast<std::size_t>(character - '0');
    }
    if (value > 99 || text != "m" + std::to_string(value)) {
        return false;
    }
    slot = value;
    return true;
}

bool has_no_more_words(std::istringstream& stream) {
    std::string extra;
    return !(stream >> extra);
}

int command_error(const std::string& message) {
    std::cerr << "error: " << message << '\n';
    return 1;
}

void print_help() {
    std::cout
        << "Expressions use +, -, *, /, ^, pi, e, and ans (the last result).\n"
        << "Functions use radians: sin cos tan asin acos atan exp ln log10 sqrt abs\n"
        << "Memory slots m0 through m99 start unset:\n"
        << "  store mN    Store ans in mN\n"
        << "  recall mN   Recall mN into ans and display it\n"
        << "  clear mN    Mark mN as unset\n"
        << "Stack commands:\n"
        << "  push        Push ans\n"
        << "  pop         Pop into ans and display it\n"
        << "  peek        Display the stack top without changing ans\n"
        << "  clear       Empty the stack and release its storage\n"
        << "  clear ans   Reset ans to 0\n"
        << "  verbose [on|off]  Enable or disable AST output for expressions\n"
        << "  help        Display this help\n";
}

int execute_repl_line(lucas::engine::Session& session, bool& verbose, const std::string& line) {
    std::istringstream stream(line);
    std::string command;
    stream >> command;

    if (command == "help") {
        if (!has_no_more_words(stream)) {
            return command_error("usage: help");
        }
        print_help();
        return 0;
    }

    if (command == "verbose") {
        std::string setting;
        if (!(stream >> setting) || setting == "on") {
            if (!has_no_more_words(stream)) {
                return command_error("usage: verbose [on|off]");
            }
            verbose = true;
            std::cout << "verbose mode enabled\n";
            return 0;
        }
        if (setting == "off" && has_no_more_words(stream)) {
            verbose = false;
            std::cout << "verbose mode disabled\n";
            return 0;
        }
        return command_error("usage: verbose [on|off]");
    }

    if (command == "store" || command == "recall") {
        std::string slot_text;
        std::size_t slot{};
        if (!(stream >> slot_text) || !has_no_more_words(stream) || !parse_memory_slot(slot_text, slot)) {
            return command_error("usage: " + command + " m0 through " + command + " m99");
        }
        const lucas::engine::Result result =
            command == "store" ? session.store(slot) : session.recall(slot);
        const int status = print_result(result);
        if (status == 0 && command == "store") {
            std::cout << "stored " << slot_text << '\n';
        }
        return status;
    }

    if (command == "push" || command == "pop" || command == "peek") {
        if (!has_no_more_words(stream)) {
            return command_error("usage: " + command);
        }
        if (command == "push") {
            const int status = print_result(session.push());
            if (status == 0) {
                std::cout << "pushed\n";
            }
            return status;
        }
        return print_result(command == "pop" ? session.pop() : session.peek());
    }

    if (command == "clear") {
        std::string target;
        if (!(stream >> target)) {
            const int status = print_result(session.clear_stack());
            if (status == 0) {
                std::cout << "stack cleared\n";
            }
            return status;
        }
        if (!has_no_more_words(stream)) {
            return command_error("usage: clear, clear ans, or clear m0 through clear m99");
        }
        if (target == "ans") {
            const int status = print_result(session.clear_ans());
            if (status == 0) {
                std::cout << "ans reset to 0\n";
            }
            return status;
        }
        std::size_t slot{};
        if (!parse_memory_slot(target, slot)) {
            return command_error("usage: clear, clear ans, or clear m0 through clear m99");
        }
        const int status = print_result(session.clear_memory(slot));
        if (status == 0) {
            std::cout << "cleared " << target << '\n';
        }
        return status;
    }

    return calculate_and_print(session, line, verbose);
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 1) {
        bool verbose = false;
        int expression_start = 1;
        while (expression_start < argc) {
            const std::string option = argv[expression_start];
            if (option == "-v" || option == "--verbose") {
                verbose = true;
                ++expression_start;
                continue;
            }
            if (option == "--") {
                ++expression_start;
            }
            break;
        }
        if (expression_start == argc) {
            return command_error("usage: lucas [-v|--verbose] <expression>");
        }

        std::ostringstream expression;
        for (int index = expression_start; index < argc; ++index) {
            if (index > expression_start) {
                expression << ' ';
            }
            expression << argv[index];
        }
        lucas::engine::Session session;
        return calculate_and_print(session, expression.str(), verbose);
    }

    std::cout << "LUCAS calculator. Type an expression, or Ctrl-D/Ctrl-Z to exit.\n";
    lucas::engine::Session session;
    bool verbose = false;
    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        if (!line.empty()) {
            execute_repl_line(session, verbose, line);
        }
    }
    return 0;
}
