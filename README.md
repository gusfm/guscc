# guscc

A simple recursive-descent C compiler written in C99, aimed at eventual self-hosting. It supports a limited subset of C: `void`/`char`/`int` types, functions, `if`, `while`, and `return`.

Given a C source file, `guscc` prints three debug sections:
1. The source with line numbers
2. The token stream produced by the lexer
3. The AST produced by the parser

> Code generation is not yet implemented.

## Requirements

- CMake 2.8+
- A C99-compatible compiler (e.g. gcc, clang)

## Compiling

```bash
cmake -B build
cmake --build build
```

The binary is placed at `build/guscc`.

## Running the sample files

Three sample C files are provided in `test/files/`.
