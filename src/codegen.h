#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "symbol_table.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <memory>

// The CodeGenContext holds all the state needed by LLVM to generate IR.
class CodeGenContext {
public:
    // 1. Context: Owns memory for core LLVM data structures (Types, Constants).
    std::unique_ptr<llvm::LLVMContext> llvmContext;
    
    // 2. Builder: A helper object that makes it easy to generate LLVM instructions.
    // It keeps track of the "current insertion point" in a BasicBlock.
    std::unique_ptr<llvm::IRBuilder<>> builder;
    
    // 3. Module: The top-level container for all IR objects (functions, global vars).
    // This is what gets compiled to machine code eventually.
    std::unique_ptr<llvm::Module> module;

    // 4. Symbol Table: Maps variable names to their LLVM memory addresses (stack slots).
    SymbolTable symTable;

    CodeGenContext() {
        llvmContext = std::make_unique<llvm::LLVMContext>();
        builder = std::make_unique<llvm::IRBuilder<>>(*llvmContext);
        module = std::make_unique<llvm::Module>("MiniCompiler", *llvmContext);
    }
    
    // Utility to generate code for the entire AST Program
    void generateCode(ProgramAST* program);
};

#endif
