# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

`guscc` is a recursive-descent C compiler written in C99, targeting x86-64 Linux assembly (System V ABI). The goal is eventual self-hosting.

## Commands

```bash
# Build
cmake -B build
cmake --build build

# Run tests with valgrind (must run from project root)
cmake --build build && cmake --build build --target valgrind

# Compile to binary (output is a.out in the current directory)
./build/guscc test/files/return_literal.c
./a.out; echo $?

# Compile to binary with explicit output name
./build/guscc -o return_literal test/files/return_literal.c
./return_literal; echo $?

# Compile to assembly only (output is <basename>.s in the current directory)
./build/guscc -S test/files/return_literal.c

# Compile with debug output (source with line numbers, token stream, AST)
./build/guscc -d test/files/return_literal.c
```

Code style is enforced by `.clang-format` (LLVM base, 100-column limit).

## Architecture

**Pipeline:** source → lexer → parser → AST + symbol table → codegen → x86-64 assembly

| Module | Role |
|--------|------|
| `src/token.{h,c}` | Token type enum and helpers |
| `src/lex.{h,c}` | Lexer — emits tokens one at a time via `lex_next()` |
| `src/ast.{h,c}` | `node_t` tagged union; debug printer |
| `src/sym.{h,c}` | Symbol table (`sym_t`/`scope_t`) and struct type registry (`struct_def_t`/`struct_member_t`) — built during parsing |
| `src/parser.{h,c}` | Recursive-descent parser with two-token lookahead; builds AST and symbol table inline |
| `src/codegen.{h,c}` | AST walker that emits x86-64 System V ABI assembly |
| `src/guscc.c` | Entry point — orchestrates the pipeline |

### Symbol table & stack frame

Parameters are assigned negative `%rbp` offsets in declaration order (first param closest to `%rbp`), followed by locals. Frame size is rounded to a multiple of 16. In the function prologue, integer parameters are spilled from `%rdi/%rsi/%rdx/%rcx/%r8/%r9` to their stack slots. Scope chains use parent pointers.

### Limits

- Max 64 functions/statements per translation unit, 8 parameters/arguments
- Only up to 6 integer parameters (System V AMD64 ABI register arguments)
- 1D fixed-size local arrays supported with braced initializers (`int a[5] = {1,2,3}`) and string literal initializers (`char s[20] = "hello"`); no multi-dimensional arrays or `sizeof(int[N])` (array in type-name context)
- Pointer arithmetic supported for local pointer variables: `+`, `-`, `+=`, `-=`, `++`, `--` (prefix and postfix), pointer comparisons, and pointer subtraction yielding element count (ptrdiff); offsets scaled by `sizeof(*p)` automatically; dereference (`*p`) respects pointee size; dereferencing a non-pointer is a compile-time error
- Pointer arithmetic on pointer-typed function parameters is not yet supported (only local pointer variables)
- Only named struct definitions; no anonymous structs, no nested struct types, no struct assignment
- Forward function calls (callee defined later) produce an "undeclared identifier" warning; forward declarations with unnamed parameters are supported
- Parenthesized abstract declarators (function pointer syntax) are parsed but not code-generated
- `switch`/`case`/`default` supported with fall-through semantics and `break`; case values must be integer literals (or negated integer literals); no computed gotos or range expressions
- Global variable declarations supported: initialized and uninitialized scalars, arrays, char arrays from string literals, and pointer-to-string globals; all use `%rip`-relative addressing; no `static`/`extern` yet; struct globals must be uninitialized

## Testing

Tests live in `test/` and use a custom macro framework (`test/ut.h`):
- `test/lex_test.c` — lexer unit tests (tokenize snippets, assert token types/values)
- `test/guscc_test.c` — end-to-end tests (compile → assemble → run → verify exit code)
- `test/files/` — `.c` test cases; `fail_*.c` files are negative tests that must cause a non-zero exit

Tests call the `guscc()` C function in-process and must be run from `build/` because test file paths are relative (`../test/files/`).
