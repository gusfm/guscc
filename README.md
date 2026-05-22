# guscc

A recursive-descent C compiler written in C99, targeting x86-64 Linux (System V ABI). It is **self-hosting**: `guscc` compiles its own source into a working compiler binary, verified by a bit-identical three-stage bootstrap (see [Self-hosting](#self-hosting)).

It compiles a subset of C — covering `void`/`char`/`short`/`int`/`long` scalar types (plus `unsigned`/`signed` specifiers), pointers, 1D arrays, structs, unions, enums, `typedef`, `static`/`extern` storage classes, `const` type qualifier, variadic function declarations (`...`), preprocessor (via `cc -E`), `sizeof`, pointer arithmetic, and the usual control flow (`if`/`else`, `switch`/`case`/`default`, `while`, `do-while`, `for`, `break`, `continue`, `return`) — directly to a native binary. Diagnostics report original source line numbers (cpp line markers are preserved and mapped back through `#include`s). By default only errors go to stderr; pass `-d` to also dump the source with line numbers, the token stream, and the AST.

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

# Or run the suite under valgrind (fails on any leak or error)
cmake --build build --target valgrind
```

Tests live in `test/` and use a custom framework (`test/ut.h`). Lexer unit tests tokenize C snippets and assert token types and values. End-to-end compiler tests compile a source file to a binary, run it, and verify the exit code. Failure-path tests verify that guscc exits non-zero on invalid input.

## Self-hosting

`guscc` can compile its own source. A three-stage bootstrap proves it has reached a fixed point:

- **stage-1** — the host compiler (gcc/clang) builds `build/guscc`.
- **stage-2** — stage-1 compiles each `src/*.c` and links them into `guscc-stage2`.
- **stage-3** — stage-2 repeats the process to produce `guscc-stage3`.

If the compiler is correct and deterministic, stage-2 and stage-3 are byte-for-byte identical:

```bash
cmake --build build --target selfhost
# builds guscc-stage2 and guscc-stage3, then `cmp -s` confirms they match
```

The bootstrap relies on `src/guscc_libc.h`, which forward-declares the libc subset the compiler uses; when `guscc` compiles its own source it sees these hand-written prototypes (gated on `__GUSCC__`) instead of the extension-laden system headers.

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

The first six integer parameters are assigned negative `%rbp` offsets in declaration order (first param closest to `%rbp`), followed by locals. Frame size is rounded to a multiple of 16. In the function prologue, those parameters are spilled from `%rdi/%rsi/%rdx/%rcx/%r8/%r9` to their stack slots. A seventh and later parameters are pushed by the caller and read from positive `%rbp` offsets, and over-six call arguments are pushed (with 16-byte stack alignment) by the caller.

## Current limitations

- Integer parameters and arguments only; no floating-point parameters (the first six use registers, extras are passed on the stack)
- 1D arrays only; no multi-dimensional arrays or `sizeof(int[5])` (array in type-name context)
- Named struct and union definitions, nested member access, struct/union assignment, and anonymous struct/union members (whose fields are promoted into the enclosing type, as `node_t` itself does) are supported; no standalone anonymous struct/union *types*
- Forward function calls (callee defined later in the file) produce an "undeclared identifier" warning; forward declarations with unnamed parameters are supported
- Variadic functions (`...`) can be declared and defined, but `va_list`/`va_start`/`va_arg` are not built-in (would come from `<stdarg.h>`)
- Parenthesized abstract declarators (function pointer syntax like `int (*)(int)`) are parsed but not code-generated
