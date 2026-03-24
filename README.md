# guscc

A simple recursive-descent C compiler written in C99, aimed at eventual self-hosting. It supports a limited subset of C: `void`/`char`/`int` types, multiple functions per file, local variables, parameters, assignments, function calls, `return`, and the full ANSI C expression syntax.

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
./build/guscc test/files/test_11.c
```

This prints the debug sections to stdout and writes the assembly to `test_11.s` in the **current directory**. The generated `.s` file can be assembled and linked with gcc:

```bash
gcc test_11.s -o test_11
./test_11; echo $?   # prints 42
```

### Example — function call

```c
int add(int a, int b)
{
    return a + b;
}

int main()
{
    return add(40, 2);
}
```

## Testing

Tests must be run from `build/` because they invoke `./guscc`:

```bash
cmake --build build && cd build && ./guscc_test
```

Tests live in `test/` and use a custom framework (`test/ut.h`). Lexer unit tests tokenize C snippets and assert token types and values. End-to-end compiler tests compile test files, assemble the output, run the resulting binary, and verify the exit code. Failure-path tests verify that guscc exits non-zero on invalid or unsupported input.

## Architecture

Pipeline: **source → lexer → parser → AST → symbol table → codegen → x86-64 assembly**

| Module | Role |
|--------|------|
| `src/token.{h,c}` | Token type and helpers |
| `src/lex.{h,c}` | Lexer — emits tokens one at a time via `lex_next()` |
| `src/ast.{h,c}` | AST node definitions (`node_t` tagged union) and debug printer |
| `src/sym.{h,c}` | Symbol table — `sym_t`/`scope_t`, built during parsing, tracks locals and params with `%rbp` offsets |
| `src/parser.{h,c}` | Recursive-descent parser with one-token lookahead; builds symbol table inline |
| `src/codegen.{h,c}` | Code generator — walks AST, emits x86-64 System V ABI assembly |
| `src/guscc.c` | Entry point — orchestrates the pipeline |

### Stack frame layout

Parameters are assigned negative `%rbp` offsets in declaration order (first param closest to `%rbp`), followed by locals. Frame size is rounded to a multiple of 16. In the function prologue, integer parameters are spilled from `%rdi/%rsi/%rdx/%rcx/%r8/%r9` to their stack slots.

## Current limitations

- `if` and `while` statement parsing are not yet implemented (stubbed)
- Array subscript (`a[i]`) and struct member access (`a.b`, `a->b`) are parsed but not yet implemented in codegen
- Functions are not tracked in the symbol table (function calls produce an "undeclared identifier" warning but work correctly)
- Global variable declarations are not yet supported
- Only up to 6 integer parameters (System V AMD64 ABI register arguments)
