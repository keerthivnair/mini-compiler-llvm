#include "common.h"
#include "ast.h"
#include "codegen.h"
#include "jit.h"
#include <iostream>
using namespace std;

// Lexer & Parser inputs
extern int yyparse();
extern FILE* yyin;

void yyerror(const char* s) {
   
    std::cerr << "Parse Error: " << s << std::endl;
}

// The global root of the AST tree created by Bison
extern ProgramAST* programRoot;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file.mc>" << std::endl;
        return 1;
    }

    // 1. Open the source code file
    FILE* file = fopen(argv[1], "r");
    if (!file) {
        std::cerr << "Could not open file: " << argv[1] << std::endl;
        return 1;
    }
    yyin = file;

    std::cout << "[1/4] Parsing Source Code..." << std::endl;
    // 2. Parse the code (Flex & Bison)
    // This blocks until parsing is complete, and populates 'programRoot'
    if (yyparse() != 0) {
        std::cerr << "Compilation failed due to syntax errors." << std::endl;
        return 1;
    }
    fclose(file);
    std::cout << "      Parse Successful. AST Built." << std::endl;

    // 3. Set up LLVM Code Generation
    std::cout << "[2/4] Initializing LLVM Codegen..." << std::endl;
    CodeGenContext context;

    // 4. Generate LLVM IR from the AST
    std::cout << "[3/4] Generating LLVM IR..." << std::endl;
    std::cout << "---------------------------------------" << std::endl;
    context.generateCode(programRoot);
    std::cout << "---------------------------------------" << std::endl;

    // 5. Execution (JIT)
    std::cout << "[4/4] JIT Compiling and Executing..." << std::endl;
    
    // Create the JIT engine. It takes ownership of the LLVM Module.
    JITEngine jit(context);

    // In a real compiler, we would look for a 'main' function.
    // For our toy compiler, we just execute whatever global code was parsed.
    // Wait, our compiler parses statements into functions or global namespace.
    // To execute, we need a defined entry point. If we defined a function 'add',
    // we can't just run it without arguments unless we build a wrapper.
    // For demonstration, let's assume the user wrote a 'main' function in sample.mc.
    jit.executeFunction("main");

    // Clean up heap allocated AST
    delete programRoot;
    
    return 0;
}
