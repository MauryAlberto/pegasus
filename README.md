# Pegasus

**Version 1.0.0** | C++17 | CMake 3.16+

Pegasus is a dynamically-typed scripting language with a bytecode compiler and stack-based virtual machine, implemented from scratch in modern C++17. The project covers the full language implementation pipeline: lexical analysis, parsing, bytecode compilation, and runtime execution. It supports first-class functions, closures, classes with single inheritance, dynamic arrays, and a small set of built-in native functions.

---

## Architecture

The implementation follows a classic single-pass compiler design with four major stages:

```
Source Code  ->  Scanner  ->  Parser  ->  Compiler  ->  Bytecode  ->  VM
```

### Scanner

The scanner (`scanner.hpp` / `scanner.cpp`) performs lexical analysis, converting raw source text into a flat stream of tokens. It handles string literals, numeric literals, identifiers, all operators, and a fixed set of reserved keywords. Comments (`//`) are silently discarded during scanning.

### Parser

The parser (`parser.hpp` / `parser.cpp`) manages token consumption and error reporting. It implements a panic-mode error recovery strategy: on a syntax error it enters panic mode, discards tokens until a synchronization point is found, and then resets to continue compiling. This allows multiple errors to be reported in a single pass rather than halting on the first one.

### Compiler

The compiler (`compiler.hpp` / `compiler.cpp`) is the core of the pipeline. It performs a direct single-pass translation from the token stream to bytecode, with no intermediate AST. Expression parsing uses a **Pratt parser** (top-down operator precedence), which handles the full operator precedence hierarchy cleanly through a table of prefix and infix parse rules.

Key responsibilities of the compiler:

- Resolves variable scope (local, global, upvalue) at compile time
- Performs **jump patching** for forward jumps in control flow
- Tracks upvalue capture chains to support closures
- Enforces immutability constraints (`immut` variables) at compile time
- Emits short (1-byte index) and long (2-byte index) variants of constant and variable instructions to handle larger programs

### VM

The virtual machine (`vm.hpp` / `vm.cpp`) is a stack-based interpreter that executes bytecode. It maintains a call stack of up to 64 frames and a value stack of up to 16,384 slots. The main execution loop is a `switch` dispatch over 62 opcodes. The VM owns all runtime object state through a set of type-specific object pools.

---

## Components

| Component | Files | Description |
|---|---|---|
| Scanner | `scanner.hpp`, `scanner.cpp` | Tokenizes source code |
| Parser | `parser.hpp`, `parser.cpp` | Token consumption and error recovery |
| Compiler | `compiler.hpp`, `compiler.cpp` | Pratt-based single-pass bytecode compiler |
| VM | `vm.hpp`, `vm.cpp` | Stack-based bytecode interpreter |
| Chunk | `chunk.hpp`, `chunk.cpp` | Bytecode and constant pool container |
| Value | `value.hpp` | Variant-based dynamic value type |
| Object System | `object.hpp`, `*_obj.hpp` | Runtime heap objects (functions, closures, classes, instances, arrays) |
| Object Pools | `*_pool.hpp` | Type-safe indexed storage for heap objects |
| Debug | `debug.hpp`, `debug.cpp` | Bytecode disassembler and diagnostic output |

---

## Object System and Memory Model

Heap-allocated objects (functions, closures, upvalues, classes, instances, bound methods, arrays) are managed through a set of **object pools** — one per type. Each pool is a contiguous `std::vector` that owns its objects. Objects are referenced throughout the interpreter by lightweight index structs (`FunctionIndex`, `ClosureIndex`, etc.) rather than raw pointers, which eliminates dangling pointer hazards and gives O(1) access with good cache locality.

String interning is handled by a dedicated `StringPool` that deduplicates `std::string_view` instances, making global variable lookup efficient without heap-allocating every identifier.

---

## Value Representation

Runtime values are represented using `std::variant`:

```
Value = std::variant<
    int, double, bool,
    std::string, std::string_view,
    std::monostate,       // nil
    FunctionIndex, NativeIndex, ClosureIndex,
    ClassIndex, InstanceIndex, BoundMethodIndex,
    ArrayIndex
>
```

This approach provides full type safety at the C++ level with no raw union casting, while `std::visit` is used throughout to handle the full set of value types uniformly.

---

## C++ Features and Techniques

- **C++17** throughout (`std::variant`, `std::string_view`, structured bindings, `if constexpr`)
- **`std::variant` and `std::visit`** for type-safe dynamic value dispatch with no raw unions
- **Pratt parsing** for operator precedence without a grammar-generating tool
- **Object pool pattern** for heap object lifetime management and cache-friendly access
- **String interning** via `std::string_view` and an unordered set
- **Move semantics** on all pool types (non-copyable, move-only)
- **Strict compiler warnings**: `-Wall -Wextra -Werror -Wshadow -Wpedantic -Wconversion -Wsign-conversion`
- **AddressSanitizer and UndefinedBehaviorSanitizer** enabled in the build
- **CMake FetchContent** for dependency management (Catch2 v3.5.4)
- **Unit testing** with Catch2 across all four major components

---

## Language Features

| Feature | Description |
|---|---|
| Mutable / Immutable variables | `mut` and `immut` declarations; immutability enforced at compile time |
| First-class functions | Functions are values; can be assigned, passed, and returned |
| Closures | Functions capture variables from enclosing scopes via upvalues |
| Classes | `class` keyword with methods, `this`, `init`, and single inheritance via `<` |
| `super` | Explicit superclass method dispatch |
| Dynamic arrays | Array literals, indexing, `.push()`, and `.len()` |
| Control flow | `if`/`else`, `while`, `for` |
| Operators | Arithmetic (`+`, `-`, `*`, `/`, `%`), comparison, logical (`and`, `or`, `!`) |
| `print` statement | Print a value followed by a newline |
| `write()` native | Print a value without a trailing newline |
| `clock()` native | Returns elapsed time in seconds as a float |
| `rand()` native | Returns a random double in [0.0, 1.0) |
| `nil`, `true`, `false` | Built-in literals |

---

## Opcodes

The VM executes a flat bytecode stream with 62 distinct opcodes covering:

- **Arithmetic**: `OP_ADD`, `OP_SUBTRACT`, `OP_MULTIPLY`, `OP_DIVIDE`, `OP_MODULO`, `OP_NEGATE`
- **Comparison**: `OP_EQUAL`, `OP_GREATER`, `OP_LESS`
- **Constant loading**: `OP_CONSTANT`, `OP_CONSTANT_LONG` (short and wide index forms)
- **Variable access**: `OP_GET_LOCAL`, `OP_SET_LOCAL`, `OP_GET_GLOBAL`, `OP_SET_GLOBAL`, `OP_DEFINE_GLOBAL`, `OP_DEFINE_GLOBAL_IMMUT` (and long variants)
- **Closures**: `OP_CLOSURE`, `OP_GET_UPVALUE`, `OP_SET_UPVALUE`, `OP_CLOSE_UPVALUE`
- **Control flow**: `OP_JUMP`, `OP_JUMP_IF_FALSE`, `OP_LOOP`
- **Functions**: `OP_CALL`, `OP_RETURN`
- **Classes**: `OP_CLASS`, `OP_METHOD`, `OP_INHERIT`, `OP_GET_PROPERTY`, `OP_SET_PROPERTY`, `OP_INVOKE`, `OP_GET_SUPER`, `OP_SUPER_INVOKE`
- **Arrays**: `OP_ARRAY`, `OP_GET_INDEX`, `OP_SET_INDEX`, `OP_ARRAY_LEN`, `OP_ARRAY_PUSH`

---

## Example: Conway's Game of Life

The following Pegasus script implements Conway's Game of Life on a 10x10 grid, demonstrating arrays, closures, nested loops, immutable constants, and multi-function composition:

```
fn randNum() {
    if(rand() % 2 > 0.5) {
        return 1;
    } else {
        return 0;
    }
}

mut grid = [];
for(mut i = 0; i < 100; i = i + 1) {
    grid.push(randNum());
}

immut GRID_ROW_SIZE = 10;
immut GRID_COL_SIZE = 10;

mut temp = [];

fn countCells(row, col) {
    immut directions = [
        [-1, -1], [-1, 0], [-1, 1],
        [0, -1],           [0, 1],
        [1, -1],  [1, 0],  [1, 1]
    ];

    mut count = 0;

    for(mut i = 0; i < directions.len(); i = i + 1) {
        immut dir = directions[i];
        immut newRow = row + dir[0];
        immut newCol = col + dir[1];

        if(newRow >= 0 and newRow < GRID_ROW_SIZE and newCol >= 0 and newCol < GRID_COL_SIZE) {
            immut index = newRow * GRID_COL_SIZE + newCol;
            if(grid[index] == 1) {
                count = count + 1;
            }
        }
    }
    return count;
}

fn checkCell(row, col) {
    immut pos = row * GRID_COL_SIZE + col;
    immut alive = grid[pos] == 1;
    immut count = countCells(row, col);

    if(alive) {
        if(count == 2 or count == 3) {
            temp[pos] = 1;
        } else {
            temp[pos] = 0;
        }
    } else {
        if(count == 3) {
            temp[pos] = 1;
        } else {
            temp[pos] = 0;
        }
    }
}

fn printGrid() {
    for(mut i = 0; i < GRID_ROW_SIZE; i = i + 1) {
        for(mut j = 0; j < GRID_COL_SIZE; j = j + 1) {
            immut pos = i * GRID_COL_SIZE + j;
            write(grid[pos]);
            write(" ");
        }
        write("\n");
    }
}

fn main() {
    while(true) {
        printGrid();
        temp = [];
        for(mut k = 0; k < 100; k = k + 1) {
            temp.push(0);
        }
        for(mut i = 0; i < GRID_ROW_SIZE; i = i + 1) {
            for(mut j = 0; j < GRID_COL_SIZE; j = j + 1) {
                checkCell(i, j);
            }
        }
        grid = temp;
    }
}

main();
```

---

## Building

**Requirements**: CMake 3.16+, a C++17-capable compiler, and an internet connection for the first build (Catch2 is fetched automatically).

```bash
mkdir build && cd build
cmake ..
make
```

This produces two targets:

- `pegasus` — the interpreter binary
- `pegasus_tests` — the Catch2 test suite

---

## Usage

Run a Pegasus script:

```bash
./pegasus path/to/script.p
```

Start an interactive REPL:

```bash
./pegasus
```

---

## Running Tests

```bash
cd build
ctest --output-on-failure
```

---

## License

MIT
