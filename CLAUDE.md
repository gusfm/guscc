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

# Skip preprocessing (for files without #include/#define)
./build/guscc -no-pp test/files/return_literal.c
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
- Preprocessor supported via external `cc -E -P`: `#include`, `#define`, `#ifdef`/`#ifndef`/`#endif`, and all standard C preprocessor directives work; preprocessing is on by default; use `-no-pp` to skip; error locations refer to preprocessed line numbers, not original source
- `sizeof` operator supported: `sizeof(type_name)` for all types (primitives, structs, enums, pointers, typedef names); `sizeof expr` for identifiers, arrays, dereferences (`sizeof *p`), member access (`sizeof s.x`), subscripts (`sizeof a[0]`), string literals, numeric literals, and cast expressions; all sizeof results are compile-time constants; no `sizeof(int[N])` (array in type-name context)
- Enums supported: named and anonymous definitions, explicit and auto-incrementing values; enumerator constants are visible in the enclosing scope; `enum` type is equivalent to `int` (4 bytes); enumerator initializers must be integer literals (no complex constant expressions)
- Named struct and union definitions supported; nested struct/union member access and struct/union assignment supported; no anonymous structs/unions
- Forward function calls (callee defined later) produce an "undeclared identifier" warning; forward declarations with unnamed parameters are supported
- Variadic functions (`...`) supported in declarations and definitions; ellipsis must follow at least one named parameter per the C grammar (`parameter_list ',' ELLIPSIS`); `va_list`/`va_start`/`va_arg` are not built-in; all calls emit `xorl %eax, %eax` (zero SSE args) for System V ABI compliance
- Parenthesized abstract declarators (function pointer syntax) are parsed but not code-generated
- `short` (2 bytes) and `long` (8 bytes) type specifiers supported; combined forms accepted: `short int`, `long int`, `long long`, `long long int`; both `long` and `long long` are 8 bytes on x86-64; arithmetic on `short`/`long` values uses 32-bit operations (values >2³¹ will truncate), but load/store correctly use the proper width (16-bit `movw`/`movswl` for `short`, 64-bit `movq` for `long`)
- `switch`/`case`/`default` supported with fall-through semantics and `break`; case values must be integer literals (or negated integer literals); no computed gotos or range expressions
- `static` storage class specifier supported: file-scope `static` gives internal linkage (omits `.globl`); block-scope `static` gives static storage duration (allocated in `.data`/`.bss` with unique label, accessed via `%rip`-relative); static local initializers must be constant expressions; no `auto`/`register` yet
- `extern` storage class specifier supported: file-scope `extern` declares external linkage without allocating storage (linker resolves); block-scope `extern` declares a variable with external linkage visible in the block; `extern` on function declarations/definitions is accepted (no-op since functions default to external linkage); `extern` with initializer is rejected
- `typedef` storage class specifier supported: type aliases for primitives, structs, enums, and pointers; chained typedefs (`typedef myint myint2`) supported; block-scope typedefs supported; typedef names are recognized as type specifiers in declarations, casts, and sizeof; no typedef for function pointer types or arrays
- `const` type qualifier supported: `const` on variable declarations prevents direct assignment, increment, and decrement (compiler error); `const` globals with initializers are placed in `.rodata`; `const` in pointer positions (`int * const p`) is parsed and the pointer is treated as const; pointer-to-const (`const int *p`) is accepted syntactically but writes through the pointer are not enforced; `const` on function parameters prevents reassignment within the function body; no `volatile` yet
- Global variable declarations supported: initialized and uninitialized scalars, arrays, char arrays from string literals, and pointer-to-string globals; all use `%rip`-relative addressing; struct globals must be uninitialized

## Testing

Tests live in `test/` and use a custom macro framework (`test/ut.h`):
- `test/lex_test.c` — lexer unit tests (tokenize snippets, assert token types/values)
- `test/guscc_test.c` — end-to-end tests (compile → assemble → run → verify exit code)
- `test/files/` — `.c` test cases; `fail_*.c` files are negative tests that must cause a non-zero exit

Tests call the `guscc()` C function in-process and must be run from `build/` because test file paths are relative (`../test/files/`).
