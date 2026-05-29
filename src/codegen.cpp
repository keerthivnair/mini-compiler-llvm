#include "codegen.h"
#include "ast.h"
#include <iostream>

using namespace llvm;

// ============================================================================
// HOW LLVM WORKS (Interview Prep)
// ============================================================================
// Every LLVM Instruction returns a `Value*`. A Value is an immutable, statically
// typed representation of data in the SSA (Static Single Assignment) form.
// You cannot "change" a Value. You can only create new ones. Variables in our
// language are represented by allocating stack memory (alloca) and issuing Load/Store.

void CodeGenContext::generateCode(ProgramAST* program) {
    // We loop through all top level statements (functions or global expressions)
    for (auto& stmt : program->getStatements()) {
        stmt->codegen(*this);
    }
    
    // Print the generated LLVM IR to the console!
    module->print(outs(), nullptr);
}

// 1. NUMBER: Just return a constant integer
Value* NumberAST::codegen(CodeGenContext& context) {
    return ConstantInt::get(*context.llvmContext, APInt(32, val, true));
}

// 2. VARIABLE: Look it up, load its value from the stack
Value* VariableAST::codegen(CodeGenContext& context) {
    Value* ptr = context.symTable.lookup(name);
    if (!ptr) {
        std::cerr << "Unknown variable name: " << name << std::endl;
        return nullptr;
    }
    // Note: In LLVM, you load FROM a pointer. The type of an int is Int32Ty.
    return context.builder->CreateLoad(Type::getInt32Ty(*context.llvmContext), ptr, name.c_str());
}

// 3. BINARY EXPRESSION: Generate Left, Generate Right, Emit Math Instruction
Value* BinaryExprAST::codegen(CodeGenContext& context) {
    // Recursively generate code for left and right branches
    Value* L = lhs->codegen(context);
    Value* R = rhs->codegen(context);
    if (!L || !R) return nullptr;

    switch (op) {
        case '+': return context.builder->CreateAdd(L, R, "addtmp");
        case '-': return context.builder->CreateSub(L, R, "subtmp");
        case '*': return context.builder->CreateMul(L, R, "multmp");
        case '/': return context.builder->CreateSDiv(L, R, "divtmp"); // Signed Division
        case '%': return context.builder->CreateSRem(L, R, "modtmp"); // Signed Remainder
        case '<': 
            L = context.builder->CreateICmpSLT(L, R, "cmptmp"); // Signed Less Than
            // LLVM comparisons return 1-bit integers (i1). We need to pad it to 32-bit (i32).
            return context.builder->CreateZExt(L, Type::getInt32Ty(*context.llvmContext), "booltmp");
        case '>': 
            L = context.builder->CreateICmpSGT(L, R, "cmptmp");
            return context.builder->CreateZExt(L, Type::getInt32Ty(*context.llvmContext), "booltmp");
        default:
            std::cerr << "Invalid binary operator." << std::endl;
            return nullptr;
    }
}

// 4. ASSIGNMENT: Allocate memory on the stack, store the value
Value* AssignmentAST::codegen(CodeGenContext& context) {
    // First, evaluate the expression on the right hand side
    Value* val = expr->codegen(context);
    if (!val) return nullptr;

    // Check if the variable already exists in the current scope
    Value* variable = context.symTable.lookup(name);
    if (!variable) {
        // Variable doesn't exist. We must ALLOCATE memory on the stack (alloca).
        // Best practice in LLVM is to put all allocas at the start of the function block.
        // For simplicity, we just put it here.
        variable = context.builder->CreateAlloca(Type::getInt32Ty(*context.llvmContext), nullptr, name.c_str());
        context.symTable.define(name, variable);
    }

    // Emit a STORE instruction to save the evaluated value into the stack memory
    context.builder->CreateStore(val, variable);
    return val;
}

// 5. FUNCTION CALL
Value* FunctionCallAST::codegen(CodeGenContext& context) {
    // Look up the function in the LLVM module
    Function* calleeF = context.module->getFunction(callee);
    if (!calleeF) {
        std::cerr << "Unknown function referenced: " << callee << std::endl;
        return nullptr;
    }

    // Generate code for all arguments
    std::vector<Value*> argsV;
    for (unsigned i = 0, e = args.size(); i != e; ++i) {
        argsV.push_back(args[i]->codegen(context));
        if (!argsV.back()) return nullptr;
    }

    return context.builder->CreateCall(calleeF, argsV, "calltmp");
}

// 6. FUNCTION DECLARATION
Value* FunctionAST::codegen(CodeGenContext& context) {
    // Create the signature: returns Int32, takes N Int32s.
    std::vector<Type*> Integers(args.size(), Type::getInt32Ty(*context.llvmContext));
    FunctionType* FT = FunctionType::get(Type::getInt32Ty(*context.llvmContext), Integers, false);
    
    // Create the LLVM function object and attach it to our module
    Function* F = Function::Create(FT, Function::ExternalLinkage, name, context.module.get());

    // Create a new Basic Block (the actual body of the function)
    BasicBlock* BB = BasicBlock::Create(*context.llvmContext, "entry", F);
    
    // Tell the Builder to start inserting instructions into this block
    context.builder->SetInsertPoint(BB);

    // Push a new variable scope for this function
    context.symTable.pushScope();

    // Map function arguments to the symbol table
    unsigned Idx = 0;
    for (auto& Arg : F->args()) {
        Arg.setName(args[Idx]);
        
        // Allocate space for the argument on the stack
        Value* Alloca = context.builder->CreateAlloca(Type::getInt32Ty(*context.llvmContext), nullptr, Arg.getName());
        
        // Store the incoming argument value into the stack slot
        context.builder->CreateStore(&Arg, Alloca);
        
        // Add to our symbol table so it can be referenced in the body
        context.symTable.define(args[Idx++], Alloca);
    }

    // Generate code for the function body
    for (auto& stmt : body) {
        stmt->codegen(context);
    }

    // Pop the variable scope (variables declared inside die here)
    context.symTable.popScope();

    // LLVM strict validation: ensure the function is well-formed
    verifyFunction(*F);
    return F;
}

// 7. RETURN STATEMENT
Value* ReturnAST::codegen(CodeGenContext& context) {
    Value* retval = expr->codegen(context);
    return context.builder->CreateRet(retval);
}

// 8. IF STATEMENT (Control Flow / Branching)
Value* IfAST::codegen(CodeGenContext& context) {
    Value* condV = condition->codegen(context);
    if (!condV) return nullptr;

    // Convert the condition to a boolean (cmp != 0)
    condV = context.builder->CreateICmpNE(
        condV, ConstantInt::get(*context.llvmContext, APInt(32, 0, true)), "ifcond");

    Function* TheFunction = context.builder->GetInsertBlock()->getParent();

    // Create blocks for the then and merge cases.
    BasicBlock* ThenBB = BasicBlock::Create(*context.llvmContext, "then", TheFunction);
    BasicBlock* MergeBB = BasicBlock::Create(*context.llvmContext, "ifcont");

    // Create the conditional branch
    context.builder->CreateCondBr(condV, ThenBB, MergeBB);

    // Populate the Then block
    context.builder->SetInsertPoint(ThenBB);
    context.symTable.pushScope(); // Scopes apply to IF blocks too!
    for (auto& stmt : thenBody) {
        stmt->codegen(context);
    }
    context.symTable.popScope();
    
    // Branch to the merge block to resume normal execution
    context.builder->CreateBr(MergeBB);

    // Resume writing code into the merge block
    TheFunction->getBasicBlockList().push_back(MergeBB);
    context.builder->SetInsertPoint(MergeBB);

    return nullptr;
}

// 9. PRINT STATEMENT
Value* PrintAST::codegen(CodeGenContext& context) {
    Value* val = expr->codegen(context);
    if (!val) return nullptr;

    std::vector<Type*> printfArgs;
    printfArgs.push_back(Type::getInt8PtrTy(*context.llvmContext)); // char*
    FunctionType* printfType = FunctionType::get(Type::getInt32Ty(*context.llvmContext), printfArgs, true); // true for varargs
    FunctionCallee printfFunc = context.module->getOrInsertFunction("printf", printfType);
    
    // Create format string "%d\n"
    Value* formatStr = context.builder->CreateGlobalStringPtr("%d\n");
    std::vector<Value*> args;
    args.push_back(formatStr);
    args.push_back(val);
    
    return context.builder->CreateCall(printfFunc, args, "printfcall");
}
