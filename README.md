# LUCAS

LUCAS (LUdwig Computer Algebra System) begins as a portable command-line smart
calculator. Its engine is library-first: the CLI is a thin wrapper over separate
frontend (lexing/parsing/AST), backend (validation/evaluation), and engine facade
libraries. A future UI should depend on `lucas_engine`, not terminal or parser
implementation details.

See [the architecture reference](docs/architecture.md) for implemented scope,
design decisions, component boundaries, and contributor workflows.

## Supported expressions

The language supports double-precision decimal or scientific literals,
parentheses, unary `+`/`-`, binary `+`, `-`, `*`, `/`, and right-associative
exponentiation (`^`). It also supports the constants `pi` and `e`, plus
lowercase one-argument functions:

```text
sin cos tan asin acos atan exp ln log10 sqrt abs
```

Trigonometric functions use radians. Function calls require parentheses and
there is no implicit multiplication.

```text
2 + 3 * 4
-(10 - 4) / .5
sin(pi / 2) + sqrt(9)
2 ^ 3 ^ 2
```

## Build and run

Ordinary builds need CMake 3.20+ and a C++20 compiler only.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/lucas "2 + 3 * 4"
```

Run `lucas` with no arguments to start the interactive REPL. The executable
location varies with the CMake generator and configuration on Windows.

## REPL memory and stack

Each REPL session starts with `ans` set to `0`. Every successful expression
updates `ans`, so `ans * 2` continues from the last result. Failed expressions
leave it unchanged.

Memory slots `m0` through `m99` start unset. `store mN` stores `ans`, `recall
mN` displays the stored value and makes it `ans`, and `clear mN` marks the slot
unset. Recalling an unset slot reports an error.

`push` saves `ans` to an on-demand stack. `pop` removes the top, displays it,
and makes it `ans`; `peek` displays the top without changing `ans`. `clear`
empties the stack and releases its allocated storage. `clear ans` resets the
last result to zero. Run `help` in the REPL for the complete command reference.
State lasts only for the current REPL session; one-shot command-line expressions
start with `ans` equal to zero.

## Verbose AST output

In the REPL, `verbose` (or `verbose on`) prints a semantic ASCII tree before
each successful expression result. `verbose off` returns to ordinary output.
For one-shot evaluation, pass `-v` or `--verbose` before the expression:

```sh
lucas --verbose "2 + sin(pi)"
```

Verbose output does not affect memory/stack commands.

## Numeric display

Displayed calculation and session values use up to 15 significant digits in
general notation. Insignificant trailing zeros are omitted, so `10.0 / 4.0`
prints `2.5`. This is a calculator-oriented display policy; it does not change
the stored `double` value or verbose AST literal labels.

```text
> verbose
verbose mode enabled
> 2 + sin(pi)
Binary(+)
+- Number(2)
`- Function(sin)
   `- Identifier(pi)
2
```

## Parser generation

`src/frontend/parser.y` is the authoritative Bison grammar and
`src/frontend/lexer.l` is the authoritative Flex lexer. Their generated C++
outputs under `src/generated/` are committed, so users and CI do not need Bison
or Flex for normal builds. Contributors changing the grammar or lexer regenerate
them with compatible Bison 3.8+ and Flex 2.6+:

```sh
cmake -S . -B build-regenerate -DLUCAS_REGENERATE_PARSER=ON
cmake --build build-regenerate
```

Commit every changed generated file with its grammar or lexer change.

The GitHub Actions matrix builds and tests the committed generated sources on
Windows, macOS, and Linux. Raspberry Pi uses the same standard CMake/C++ build
on its ARM Linux toolchain; no runtime or package dependency is required.
