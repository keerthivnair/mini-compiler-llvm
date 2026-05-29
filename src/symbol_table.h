#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

// Forward declaration of LLVM Value because our symbol table will actually 
// map variable names directly to their LLVM memory addresses (Value*) 
// during the Codegen phase!
namespace llvm {
    class Value;
}

// A single scope block (like inside a function or an if-statement)
class Scope {
public:
    std::unordered_map<std::string, llvm::Value*> locals;
};

// The Symbol Table manages a stack of Scopes
class SymbolTable {
private:
    std::vector<std::unique_ptr<Scope>> scopeStack;

public:
    SymbolTable() {
        // Push the global scope
        pushScope();
    }

    void pushScope() {
        scopeStack.push_back(std::make_unique<Scope>());
    }

    void popScope() {
        if (scopeStack.size() > 1) {
            scopeStack.pop_back();
        }
    }

    // Assign a variable to the CURRENT scope
    void define(const std::string& name, llvm::Value* value) {
        scopeStack.back()->locals[name] = value;
    }

    // Look up a variable starting from the innermost scope and going outwards
    llvm::Value* lookup(const std::string& name) {
        // Iterate backwards from the top of the stack (innermost scope)
        for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
            auto loc = (*it)->locals.find(name);
            if (loc != (*it)->locals.end()) {
                return loc->second;
            }
        }
        return nullptr; // Variable not found!
    }
};

#endif
