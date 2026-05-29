#ifndef JIT_H
#define JIT_H

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Support/TargetSelect.h>
#include "codegen.h"

class JITEngine {
private:
    llvm::ExecutionEngine* engine;

public:
    JITEngine(CodeGenContext& context);
    ~JITEngine();

    // Look up a compiled function by name and execute it!
    void executeFunction(const std::string& name);
};

#endif
