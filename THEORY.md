# 🧠 Deep Compiler Theory & Architecture

This document contains a rigorous, interview-level breakdown of the computer science theory and systems engineering decisions driving this compiler.

---

## 1. The Frontend: Flex & Bison

### Lexical Analysis (Flex)
Flex converts a stream of characters into a stream of tokens using a **Deterministic Finite Automaton (DFA)**.
* **Theory**: Flex takes Regular Expressions (like `[a-zA-Z]+`) and converts them into a state machine. DFAs guarantee `O(N)` linear time scanning of the input file.
* **Memory Ownership (The `strdup` trap)**: When Flex matches an identifier like `x`, that string is stored in a global buffer called `yytext`. If we pass `yytext` directly to the AST, the string will be overwritten when Flex reads the next token! Therefore, in `lexer.l`, we use `strdup(yytext)` to allocate new heap memory and copy the string. We then transfer ownership of this memory to the Parser.

### Syntax Analysis (Bison)
Bison is an **LALR(1)** (Look-Ahead Left-to-Right, Rightmost derivation) parser generator.
* **Theory**: It maintains a stack. As it receives tokens from Flex, it pushes them onto the stack (**Shift**). When the top of the stack matches a grammatical rule (e.g., `expr + expr`), it pops them off and replaces them with a single parent node (**Reduce**).
* **Shift/Reduce Conflicts**: In an expression like `2 + 3 * 4`, if the parser sees `2 + 3` and the next token is `*`, should it Reduce `2 + 3` immediately, or Shift `*` onto the stack? Reducing immediately creates `(2+3)*4=20` (Wrong). Shifting creates `2+(3*4)=14` (Correct). We solve this by explicitly defining Operator Precedence in Bison (`%left T_PLUS`, `%left T_MUL`).
* **AST Construction**: During a Reduction, Bison executes our custom C++ code. This is where we call `new BinaryExprAST(...)`, effectively translating the transient parser stack into a permanent Heap-allocated Abstract Syntax Tree.

---

## 2. The Middle-end: AST & Memory Management

### Polymorphism & The Visitor Pattern
Our AST is a tree of highly diverse objects (Variables, Binary Operations, If Statements). We use Polymorphism (a base `ASTNode` class with a pure virtual `codegen()` method) so that the parent node does not need to know the exact type of its children. It simply loops over them and calls `child->codegen()`.

### RAII and `std::unique_ptr`
A compiler is extremely prone to memory leaks because it allocates thousands of nodes.
* **Architecture**: A tree structure represents **Exclusive Ownership**. A parent node owns its children. No two parents share the same child.
* **Implementation**: By using `std::unique_ptr<ASTNode>` for all child pointers, we guarantee that when the root `ProgramAST` is destroyed at the end of `main.cpp`, the destructor will recursively cascade down the tree, automatically deleting every node without a single manual `delete` call.

### The Symbol Table (Semantic Analysis)
* **Architecture**: A Stack of Hash Maps.
* **Why a Hash Map?** Variable lookups must be `O(1)` fast. `std::unordered_map` provides this.
* **Why a Stack?** Lexical Scoping. When the compiler enters a `{` block (like a function or an `if` statement), it pushes a new Hash Map onto the stack. All variables declared here live in this map. When the compiler hits `}`, it pops the map, instantly destroying the local variables. When resolving a variable (`lookup`), the compiler searches backwards from the top of the stack to the bottom, allowing local variables to "shadow" global ones.

---

## 3. The Backend: LLVM IR & SSA

### Static Single Assignment (SSA) Form
LLVM Intermediate Representation requires that every variable (register) is assigned exactly **once**. You cannot do `%1 = 1; %1 = 2;`.
* **The Problem**: Our language allows variable reassignment (`x = 10; x = 20;`). How do we compile this into LLVM?
* **The Solution (Memory vs Registers)**: We bypass SSA by allocating memory on the Call Stack. When we declare a variable, we emit an `alloca` instruction to reserve RAM. When we assign it, we emit a `store` instruction to write to that RAM. When we read it, we emit a `load` instruction. Because the LLVM *register* holding the memory address never changes, we satisfy SSA. LLVM's optimizer pass (`mem2reg`) will later realize the memory is localized and optimize it into raw CPU registers.

### Control Flow Graphs (CFG)
How does an `if` statement compile to assembly?
* **Basic Blocks**: A BasicBlock is a linear sequence of instructions with exactly one entry point and one exit point. 
* **Branching**: For an `if` statement, we create three blocks: `Entry`, `Then`, and `Merge`. We emit a Conditional Branch (`br i1 %cond, label %Then, label %Merge`). The `Then` block ends with an Unconditional Branch to `Merge`. This creates a mathematical graph in memory that LLVM flattens into assembly `jmp` and `je` instructions.

---

## 4. Execution: The JIT Compiler

Our compiler does not output an `a.out` binary file (Ahead-Of-Time compilation). Instead, it executes the code immediately (Just-In-Time).
* **MCJIT Engine**: We pass ownership of our LLVM Module to the Machine Code JIT Engine.
* **How it works**: The JIT allocates pages in the host machine's RAM. It translates the LLVM IR down to native host assembly (e.g., x86_64). It writes this assembly into the RAM. It uses the OS system call `mprotect` to mark those RAM pages as Executable (`PROT_EXEC`).
* **Execution**: We ask the JIT for a function pointer to our compiled `main` function. We then execute that C++ function pointer. The CPU's Instruction Pointer jumps directly into the dynamically generated assembly in RAM, running our custom language at the exact same speed as native C++.
