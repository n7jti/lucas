# LUCAS architecture reference

## Purpose and current status

LUCAS (LUdwig Computer Algebra System) is a portable, library-first command-line
scientific calculator. It is the foundation for a future computer algebra system
and graphical UI. The implemented product supports arithmetic, scientific
functions, calculator-session state, optional ASCII AST output, and one-shot or
interactive CLI use.

The current implementation is intentionally a numeric `double` calculator, not
yet a symbolic algebra system. Variables beyond `ans`, persisted state,
arbitrary-precision values, multi-argument functions, degree modes, and a UI are
deferred.

## Goals and design decisions

| Decision | Rationale |
| --- | --- |
| C++20 with CMake | Cross-platform toolchain support on Windows, macOS, Linux, and Raspberry Pi. |
| Library-first layering | A future UI uses engine APIs instead of duplicating CLI, parser, or evaluator logic. |
| Bison grammar plus Flex lexer | Formal, maintainable syntax definition with a distinct lexer and parser. |
| Committed generated parser sources | Ordinary builds need only CMake and a C++ compiler; Bison/Flex are contributor-only regeneration tools. |
| Separate parse, validate, evaluate stages | Each stage is testable and reports diagnostics without terminal I/O or process termination. |
| `double` values | A portable, standard-library-only starting point that can later be replaced behind a value abstraction. |
| Radians | Matches C++ standard math functions and conventional programming/scientific behavior. |
| 15-significant-digit CLI output | Calculator-like display that retains useful precision while suppressing insignificant trailing zeros. |

## Component structure

### Frontend: `lucas_frontend`

The frontend owns source spans, tokens, Bison/Flex integration, and the AST.
`src/frontend/parser.y` is the authoritative grammar; `src/frontend/lexer.l` is
the authoritative lexer. Parser actions build AST nodes and do not perform
evaluation.

AST nodes represent number literals, unary operations, binary operations,
identifiers, and one-argument function calls. The frontend also owns
`render_ascii_tree`, a terminal-independent renderer with semantic labels:

```text
Binary(+)
+- Number(2)
`- Function(sin)
   `- Identifier(pi)
```

### Backend: `lucas_backend`

The backend validates AST shape and supported names before evaluation. It
evaluates valid ASTs using `<cmath>` and returns either a `double` value or
structured diagnostics. It has no dependency on terminal I/O, CLI command
syntax, or session storage.

Validation accepts the built-in constants `pi`, `e`, and the engine-provided
`ans` value. Evaluation reports deliberate errors for invalid real-domain
operations such as division by zero, negative square roots, invalid logarithm
arguments, out-of-range inverse trigonometry, undefined tangent, and invalid
negative-base powers.

### Engine: `lucas_engine`

The engine coordinates parse, validate, and evaluate. A stateless `calculate`
entry point uses `ans = 0`. `Session` adds reusable interactive state:

- `ans` starts at `0` and updates only after successful calculations.
- Memory slots `m0` through `m99` are optional and initially unset.
- `store` saves `ans`; `recall` restores a slot into `ans`; `clear mN` unsets a
  slot.
- An on-demand `std::vector<double>` stack supports push, pop, peek, and a
  capacity-releasing clear.
- Failed calculations or state operations preserve `ans`.

The engine can opt into returning the ASCII AST for the exact parsed expression
it evaluates; it does not reparse input for verbose output.

### CLI: `lucas`

The CLI is a thin terminal wrapper over `Session`. With no arguments, it starts
one REPL session. One-shot invocation creates a fresh session, so `ans` begins
at `0`.

The REPL commands are:

```text
store mN       Store ans in m0 through m99
recall mN      Display mN and make it ans
clear mN       Mark mN unset
push           Push ans
pop            Pop into ans and display it
peek           Display the stack top without changing ans
clear          Empty the stack and release its storage
clear ans      Reset ans to 0
verbose [on|off]
help
```

`verbose` enables semantic AST output before each successful REPL expression
result; `verbose off` disables it. For one-shot expressions, use `-v` or
`--verbose`, for example `lucas --verbose "2 + sin(pi)"`.

## Expression language

The language uses lowercase, case-sensitive names and requires parentheses for
function calls. There is no implicit multiplication.

```text
2 + 3 * 4
-(10 - 4) / .5
2 ^ 3 ^ 2
sin(pi / 2) + sqrt(9)
ans * 2
```

Supported operators are unary `+`/`-`, binary `+`, `-`, `*`, `/`, and
right-associative `^`. Built-in constants are `pi` and `e`. The functions are:

```text
sin cos tan asin acos atan exp ln log10 sqrt abs
```

Trigonometric functions use radians. Operator precedence is formalized in the
Bison grammar, including the intended behavior `-2 ^ 2 == -4` and
`2 ^ 3 ^ 2 == 512`.

## Numeric display

The evaluator retains `double` values. At the CLI boundary, result-bearing
operations use general notation with up to 15 significant digits. General
formatting omits insignificant trailing zeros; for example, `10.0 / 4.0`
displays `2.5`. Very small or large values may use exponent notation. This is a
display policy only: stored values remain unchanged, and verbose `Number(...)`
AST labels retain their frontend formatting.

## Build, generation, and tests

Normal builds require CMake 3.20+ and a C++20 compiler:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Bison 3.8+ and Flex 2.6+ are required only when grammar or lexer source changes:

```sh
cmake -S . -B build-regenerate -DLUCAS_REGENERATE_PARSER=ON
cmake --build build-regenerate
```

Commit regenerated `src/generated/` files alongside changes to `parser.y` or
`lexer.l`. CI builds and tests the committed generated sources on Windows,
macOS, and Linux. The same dependency-light CMake build applies to Raspberry Pi
on its ARM Linux toolchain.

Tests cover expression parsing/evaluation, scientific domain diagnostics,
session-state behavior, AST rendering, and CLI one-shot output. Add focused
tests whenever language, evaluation, state, renderer, or CLI behavior changes.
