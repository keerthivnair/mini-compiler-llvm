# Technical Interview Q&A

This document prepares you for deep technical interviews (NVIDIA, Apple, etc.) by providing rigorous questions about this compiler's architecture, memory, and LLVM interactions.

## Phase 1: Architecture & Dependencies

**Q: Why use Flex and Bison instead of writing a hand-rolled recursive descent parser?**
* **Short Answer:** Speed of development and standard LALR(1) parsing guarantees.
* **Deep Technical Answer:** A hand-written recursive descent parser (LL(1) or LL(k)) is excellent for generating precise error messages and avoiding shift/reduce conflicts, which is why GCC and Clang switched to them. However, Flex/Bison generates a Deterministic Finite Automaton (DFA) for lexing and an LALR(1) state machine for parsing. The generated state machine shift/reduces in linear time `O(n)`. For a small prototype or a new DSL, Bison provides a rigorous mathematical proof that the grammar is unambiguous (if no conflicts exist).
* **Interviewer Trap:** They might ask "Is Clang written in Bison?" No. Clang uses a hand-written recursive descent parser for better diagnostics and C++'s complex parsing rules (which are not context-free, e.g., the "most vexing parse").
* **What they are testing:** Do you know the difference between LL vs LR parsers, and do you know how real-world industrial compilers work?

**Q: How does memory flow from the Lexer to the Parser?**
* **Short Answer:** The Lexer passes semantic values to the Parser via a global/injected union called `yylval`.
* **Deep Technical Answer:** Flex identifies a regex match and returns a token ID (an `int`). But for things like identifiers or integers, the *value* of the token must be extracted. We copy the string or parse the integer and store it in `yylval`. `yylval` is typically a `union` (or `std::variant` in modern C++). The parser then shifts this token onto its stack. When a rule reduces, the parser pops these semantic values off the stack and uses them to allocate heap memory for an AST Node, transferring ownership.
* **What they are testing:** Stack vs Heap, variable lifetime, and C/C++ union/variant usage.
