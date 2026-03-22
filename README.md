# guscc

A simple recursive-descent C compiler written in C99, aimed at eventual self-hosting. It supports a limited subset of C: `void`/`char`/`int` types, functions, `return`, and the full ANSI C expression syntax.

Given a C source file, `guscc` compiles it to x86-64 assembly (`.s`) and prints three debug sections to stdout:
1. The source with line numbers
2. The token stream produced by the lexer
3. The AST produced by the parser

## Requirements

- CMake 2.8+
- A C99-compatible compiler (e.g. gcc, clang)

## Building

```bash
cmake -B build
cmake --build build
```

The binary is placed at `build/guscc`.

## Usage

```bash
./build/guscc test/files/test_1.c
```

This prints the debug sections to stdout and writes the assembly to `test_1.s` in the **current directory**. The generated `.s` file can be assembled and linked with gcc:

```bash
gcc test_1.s -o test_1
./test_1; echo $?
```

## Testing

```bash
cmake --build build && ctest
# or run directly
./build/guscc_test
```

Tests live in `test/` and use a custom framework (`test/ut.h`). Lexer unit tests tokenize C snippets and assert token types and values. End-to-end compiler tests compile test files, assemble the output, run the resulting binary, and verify the exit code.

## Architecture

Pipeline: **source → lexer → parser → AST → codegen → x86-64 assembly**

| Module | Role |
|--------|------|
| `src/token.{h,c}` | Token type and helpers |
| `src/lex.{h,c}` | Lexer — emits tokens one at a time via `lex_next()` |
| `src/ast.{h,c}` | AST node definitions (`node_t` tagged union) and debug printer |
| `src/parser.{h,c}` | Recursive-descent parser with one-token lookahead |
| `src/codegen.{h,c}` | Code generator — walks AST, emits x86-64 assembly |
| `src/guscc.c` | Entry point — orchestrates the pipeline |

## Current limitations

- `if` and `while` statement parsing are not yet implemented (stubbed)
- Identifier references, assignment, postfix `++`/`--`, array subscript, function calls, and struct member access are parsed but not yet implemented in codegen (require a symbol table and stack frame management)
