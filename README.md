# 🚀 MiniCompiler: An LLVM & Flex/Bison JIT Compiler

A complete, beginner-friendly, deeply educational C-like compiler built from scratch using **C++**, **Flex**, **Bison**, and **LLVM 14**. 

This project was specifically designed to demonstrate real-world systems engineering, compiler architecture, and memory management for highly technical interviews (e.g., NVIDIA, Apple, AMD). It does not just interpret code; it parses, analyzes, generates Intermediate Representation (IR), and Just-In-Time (JIT) compiles it down to raw machine code in memory.

---

## 🏗️ What is this project doing?
This project transforms human-readable source code into executable machine code in milliseconds. It implements a complete end-to-end compiler pipeline:

1. **Lexical Analysis**: Reads raw characters and groups them into Tokens.
2. **Syntax Analysis**: Groups tokens into mathematical and logical structures.
3. **AST Construction**: Builds a polymorphic, heap-allocated tree representing the program.
4. **Semantic Analysis**: Manages variable scopes and tracks function declarations.
5. **LLVM IR Generation**: Translates the AST into a machine-independent assembly language.
6. **JIT Execution**: Translates LLVM IR into x86/ARM machine code and executes it directly on the CPU.

---

## 📂 Project Structure: Why Every File Exists

### Build System
* **`CMakeLists.txt`**: **Why is it needed?** LLVM is a massive, multi-gigabyte library broken into dozens of sub-components. If we compiled via command line (`g++ main.cpp -lLLVM...`), we'd have to link dozens of LLVM libraries manually. CMake uses `find_package(LLVM)` to automatically locate the LLVM installation on your machine, extract the correct compiler flags, and link exactly the components we need (like `MCJIT`, `ExecutionEngine`, `Core`). It also automatically hooks into Flex and Bison to generate C++ code before compiling `main.cpp`.

### The Frontend (Lexing & Parsing)
* **`src/lexer.l`**: The Flex file. It uses Regular Expressions to define what an integer, identifier, or symbol looks like. Flex generates a Deterministic Finite Automaton (DFA) that scans the source file and returns Integer IDs (Tokens) to the parser.
* **`src/parser.y`**: The Bison file. It defines the Context-Free Grammar (CFG) of our language (e.g., an `expr` is an `expr + expr`). It is a Shift-Reduce LALR(1) parser. When it successfully matches a grammar rule, it dynamically allocates AST Nodes on the heap.

### The Middle-end (AST & Semantic Analysis)
* **`src/ast.h` / `src/ast.cpp`**: Defines the Abstract Syntax Tree (AST). It uses polymorphism (a base `ASTNode` class) so the compiler can hold a generic tree of expressions. It uses `std::unique_ptr` exclusively to guarantee memory safety and prevent leaks when the tree is destroyed.
* **`src/symbol_table.h` / `src/symbol_table.cpp`**: The Symbol Table tracks variables. It is implemented as a Stack of Hash Maps (`std::vector<std::unordered_map>`). This allows the compiler to handle nested scopes (like variables inside an `if` block shadowing variables outside of it).

### The Backend (LLVM IR & JIT)
* **`src/codegen.h` / `src/codegen.cpp`**: The heart of the compiler. It iterates over the AST and uses the `llvm::IRBuilder` to emit LLVM Intermediate Representation. It maps high-level concepts (like variable reassignment) into low-level LLVM concepts (like stack memory allocation via `alloca`).
* **`src/jit.h` / `src/jit.cpp`**: The Execution Engine. It takes the generated LLVM IR, allocates executable memory pages in RAM, compiles the IR into raw CPU assembly (x86/ARM), and executes it natively as a C++ function pointer.

### The Driver
* **`src/main.cpp`**: The entry point. It ties the entire pipeline together: `fopen()` -> `yyparse()` -> `AST` -> `CodeGen` -> `JIT`.
* **`src/common.h`**: A shared header to prevent circular dependencies between Flex, Bison, and the AST.

---

## ⚙️ How to Build and Run

### Prerequisites
You must have a C++17 compiler, CMake, Flex, Bison, and LLVM 14 installed.
```bash
sudo apt-get update
sudo apt-get install cmake flex bison llvm-14-dev
```

### Build Instructions
We use an out-of-source build (the `build/` directory) to keep our source tree clean.
```bash
mkdir build
cd build
cmake ..
make
```

### Running the Compiler
```bash
./minicompiler ../examples/sample.mc
```

---

## 🖥️ Sample Output

Given the `sample.mc` file:
```cpp
func add(a, b) {
    return a + b;
}

func main() {
    x = 10;
    y = 20;

    if (x < y) {
        print(add(x, y));
    }
    return 0;
}
```

The compiler will parse it, generate LLVM IR, and output:
```llvm
[1/4] Parsing Source Code...
      Parse Successful. AST Built.
[2/4] Initializing LLVM Codegen...
[3/4] Generating LLVM IR...
---------------------------------------
; ModuleID = 'MiniCompiler'
source_filename = "MiniCompiler"

@0 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 10, i32* %x, align 4
  %y = alloca i32, align 4
...
---------------------------------------
[4/4] JIT Compiling and Executing...

=== EXECUTING main() ===
30
=== EXECUTION FINISHED ===
```
}
