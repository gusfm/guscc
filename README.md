# guscc

A recursive-descent C compiler written in C99, targeting x86-64 Linux (System V ABI). The goal is eventual self-hosting.

It compiles a subset of C — covering `void`/`char`/`int` scalar types, pointers, 1D arrays, structs, pointer arithmetic, and the usual control flow (`if`/`else`, `while`, `do-while`, `for`, `break`, `continue`, `return`) — down to x86-64 assembly (`.s`). By default only errors go to stderr; pass `-d` to also dump the source with line numbers, the token stream, and the AST.

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
# Compile silently (only errors to stderr)
./build/guscc test/files/return_literal.c

# Compile with debug output (source, tokens, AST printed to stdout)
./build/guscc -d test/files/return_literal.c
```

Both write the assembly to `return_literal.s` in the **current directory**. Assemble and run with gcc:

```bash
gcc return_literal.s -o return_literal
./return_literal; echo $?   # prints 42
```

## Testing

Tests must be run from `build/` because they invoke `./guscc`:

```bash
cmake --build build && cd build && ./guscc_test
```

Tests live in `test/` and use a custom framework (`test/ut.h`). Lexer unit tests tokenize C snippets and assert token types and values. End-to-end compiler tests compile a source file, assemble the output, run the binary, and verify the exit code. Failure-path tests verify that guscc exits non-zero on invalid input.

## Architecture

Pipeline: **source → lexer → parser → AST + symbol table → codegen → x86-64 assembly**

| Module | Role |
|--------|------|
| `src/token.{h,c}` | Token type and helpers |
| `src/lex.{h,c}` | Lexer — emits tokens one at a time via `lex_next()` |
| `src/ast.{h,c}` | AST node definitions (`node_t` tagged union) and debug printer |
| `src/sym.{h,c}` | Symbol table — `sym_t`/`scope_t`, built during parsing, tracks locals and params with `%rbp` offsets |
| `src/parser.{h,c}` | Recursive-descent parser with one-token lookahead; builds AST and symbol table inline |
| `src/codegen.{h,c}` | Code generator — walks AST, emits x86-64 System V ABI assembly |
| `src/guscc.c` | Entry point — orchestrates the pipeline |

### Stack frame layout

Parameters are assigned negative `%rbp` offsets in declaration order (first param closest to `%rbp`), followed by locals. Frame size is rounded to a multiple of 16. In the function prologue, integer parameters are spilled from `%rdi/%rsi/%rdx/%rcx/%r8/%r9` to their stack slots.

## Current limitations

- No global variable declarations
- Max 6 integer parameters (System V AMD64 ABI register arguments); no floating-point parameters
- Pointer arithmetic only works on local pointer variables, not pointer-typed parameters
- 1D arrays only; no multi-dimensional arrays or `sizeof(int[5])` (array in type-name context)
- Only named struct definitions; no anonymous structs, no nested struct types, no struct assignment
- Forward function calls (callee defined later in the file) produce an "undeclared identifier" warning; forward declarations with unnamed parameters are supported
- Parenthesized abstract declarators (function pointer syntax like `int (*)(int)`) are parsed but not code-generated
