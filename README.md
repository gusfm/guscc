# guscc

A recursive-descent C compiler written in C99, targeting x86-64 Linux (System V ABI). The goal is eventual self-hosting.

It compiles a subset of C — covering `void`/`char`/`int` scalar types, pointers, 1D arrays, structs, unions, enums, `typedef`, `static`/`extern` storage classes, `const` type qualifier, variadic function declarations (`...`), preprocessor (via `cc -E`), `sizeof`, pointer arithmetic, and the usual control flow (`if`/`else`, `switch`/`case`/`default`, `while`, `do-while`, `for`, `break`, `continue`, `return`) — directly to a native binary. By default only errors go to stderr; pass `-d` to also dump the source with line numbers, the token stream, and the AST.

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
# Compile to binary (output is a.out in the current directory)
./build/guscc test/files/return_literal.c
./a.out; echo $?   # prints 42

# Compile to binary with explicit output name
./build/guscc -o return_literal test/files/return_literal.c
./return_literal; echo $?   # prints 42

# Compile to assembly only (output is return_literal.s in the current directory)
./build/guscc -S test/files/return_literal.c

# Compile with debug output (source, tokens, AST printed to stdout)
./build/guscc -d test/files/return_literal.c

# Skip preprocessing (for files without #include/#define)
./build/guscc -no-pp test/files/return_literal.c
```

## Testing

Tests must be run from `build/` because test file paths are relative (`../test/files/`):

```bash
cmake --build build && cd build && ./guscc_test
```

Tests live in `test/` and use a custom framework (`test/ut.h`). Lexer unit tests tokenize C snippets and assert token types and values. End-to-end compiler tests compile a source file to a binary, run it, and verify the exit code. Failure-path tests verify that guscc exits non-zero on invalid input.

## Architecture

Pipeline: **source → lexer → parser → AST + symbol table → codegen → x86-64 assembly → binary**

| Module | Role |
|--------|------|
| `src/token.{h,c}` | Token type and helpers |
| `src/lex.{h,c}` | Lexer — emits tokens one at a time via `lex_next()` |
| `src/ast.{h,c}` | AST node definitions (`node_t` tagged union) and debug printer |
| `src/sym.{h,c}` | Symbol table — `sym_t`/`scope_t`, built during parsing, tracks locals and params with `%rbp` offsets |
| `src/parser.{h,c}` | Recursive-descent parser with two-token lookahead; builds AST and symbol table inline |
| `src/codegen.{h,c}` | Code generator — walks AST, emits x86-64 System V ABI assembly |
| `src/guscc.c` | Entry point — orchestrates the pipeline |

### Stack frame layout

Parameters are assigned negative `%rbp` offsets in declaration order (first param closest to `%rbp`), followed by locals. Frame size is rounded to a multiple of 16. In the function prologue, integer parameters are spilled from `%rdi/%rsi/%rdx/%rcx/%r8/%r9` to their stack slots.

## Current limitations

- Max 6 integer parameters (System V AMD64 ABI register arguments); no floating-point parameters
- 1D arrays only; no multi-dimensional arrays or `sizeof(int[5])` (array in type-name context)
- Named struct and union definitions supported; nested struct/union member access and struct/union assignment supported; no anonymous structs/unions
- Forward function calls (callee defined later in the file) produce an "undeclared identifier" warning; forward declarations with unnamed parameters are supported
- Variadic functions (`...`) can be declared and defined, but `va_list`/`va_start`/`va_arg` are not built-in (would come from `<stdarg.h>`)
- Parenthesized abstract declarators (function pointer syntax like `int (*)(int)`) are parsed but not code-generated
