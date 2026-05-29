#include "jit.h"
#include <iostream>

using namespace llvm;

JITEngine::JITEngine(CodeGenContext& context) {
    // 1. Initialize the target architecture (x86, ARM, etc)
    // This allows LLVM to know what machine code to generate
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    // 2. We must "extract" the module from our context
    // because the ExecutionEngine takes ownership of it.
    std::unique_ptr<Module> Owner = std::move(context.module);
    std::string errStr;

    // 3. Create the MCJIT Execution Engine
    // It takes our LLVM IR Module and compiles it in RAM
    engine = EngineBuilder(std::move(Owner))
                .setErrorStr(&errStr)
                .setEngineKind(EngineKind::JIT)
                .create();

    if (!engine) {
        std::cerr << "Failed to construct ExecutionEngine: " << errStr << std::endl;
        exit(1);
    }

    // 4. Force compilation of all functions
    engine->finalizeObject();
}

JITEngine::~JITEngine() {
    delete engine;
}

void JITEngine::executeFunction(const std::string& name) {
    // Look up the function pointer in the compiled machine code
    Function* func = engine->FindFunctionNamed(name);
    
    if (!func) {
        std::cerr << "JIT Error: Could not find function '" << name << "' to execute." << std::endl;
        return;
    }

    std::cout << "\n=== EXECUTING " << name << "() ===\n";
    
    // Execute the function! No arguments are passed for simplicity.
    std::vector<GenericValue> noargs;
    GenericValue result = engine->runFunction(func, noargs);
    
    std::cout << "=== EXECUTION FINISHED ===\n";
}
