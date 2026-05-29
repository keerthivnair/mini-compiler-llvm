#ifndef AST_H
#define AST_H

#include <string>
#include <memory>
#include <vector>

// Forward declarations for LLVM types that we will use in Phase 6
namespace llvm {
    class Value;
}
class CodeGenContext; // We will define this in Phase 6

// ============================================================================
// BASE AST NODE
// ============================================================================
// Why do we need a base class?
// Because our parser will build a tree of diverse nodes (Variables, Binary Operations, etc.)
// but we need a single type to hold them in our Parser Stack. Polymorphism allows us
// to call a virtual 'codegen()' function on ANY node without knowing its specific type.
class ASTNode {
public:
    virtual ~ASTNode() = default;
    
    // The core of the compiler: every node must know how to generate its own LLVM IR
    virtual llvm::Value* codegen(CodeGenContext& context) = 0;
};

// ============================================================================
// EXPRESSION NODES
// ============================================================================

class NumberAST : public ASTNode {
    int val;
public: 
    NumberAST(int val) : val(val) {}
    llvm::Value* codegen(CodeGenContext& context) override;

    int getIntValue() const { return val; }
};

class VariableAST : public ASTNode {
    std::string name;
public:
    VariableAST(const std::string& name) : name(name) {}
    const std::string& getName() const { return name; }
    llvm::Value* codegen(CodeGenContext& context) override;
};

class BinaryExprAST : public ASTNode {
    char op;
    std::unique_ptr<ASTNode> lhs, rhs;
public:
    // Ownership: The Binary node takes UNIQUE ownership of its Left and Right children.
    // When this node is destroyed, its children are recursively destroyed.
    BinaryExprAST(char op, std::unique_ptr<ASTNode> lhs, std::unique_ptr<ASTNode> rhs)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    
    llvm::Value* codegen(CodeGenContext& context) override;
};

class FunctionCallAST : public ASTNode {
    std::string callee;
    std::vector<std::unique_ptr<ASTNode>> args;
public:
    FunctionCallAST(const std::string& callee, std::vector<std::unique_ptr<ASTNode>> args)
        : callee(callee), args(std::move(args)) {}
    
    llvm::Value* codegen(CodeGenContext& context) override;
};

// ============================================================================
// STATEMENT NODES
// ============================================================================

class ReturnAST : public ASTNode {
    std::unique_ptr<ASTNode> expr;
public:
    ReturnAST(std::unique_ptr<ASTNode> expr) : expr(std::move(expr)) {}
    llvm::Value* codegen(CodeGenContext& context) override;
};

class PrintAST : public ASTNode {
    std::unique_ptr<ASTNode> expr;
public:
    PrintAST(std::unique_ptr<ASTNode> expr) : expr(std::move(expr)) {}
    llvm::Value* codegen(CodeGenContext& context) override;
};

class AssignmentAST : public ASTNode {
    std::string name;
    std::unique_ptr<ASTNode> expr;
public:
    AssignmentAST(const std::string& name, std::unique_ptr<ASTNode> expr)
        : name(name), expr(std::move(expr)) {}
    llvm::Value* codegen(CodeGenContext& context) override;
};

class IfAST : public ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::vector<std::unique_ptr<ASTNode>> thenBody;
public:
    IfAST(std::unique_ptr<ASTNode> cond, std::vector<std::unique_ptr<ASTNode>> thenBody)
        : condition(std::move(cond)), thenBody(std::move(thenBody)) {}
    llvm::Value* codegen(CodeGenContext& context) override;
};

// ============================================================================
// TOP LEVEL NODES
// ============================================================================

class FunctionAST : public ASTNode {
    std::string name;
    std::vector<std::string> args;
    std::vector<std::unique_ptr<ASTNode>> body;
public:
    FunctionAST(const std::string& name, std::vector<std::string> args, std::vector<std::unique_ptr<ASTNode>> body)
        : name(name), args(args), body(std::move(body)) {}
    
    llvm::Value* codegen(CodeGenContext& context) override;
};

class ProgramAST {
    std::vector<std::unique_ptr<ASTNode>> statements;
public:
    void addStatement(std::unique_ptr<ASTNode> stmt) {
        statements.push_back(std::move(stmt));
    }
    const std::vector<std::unique_ptr<ASTNode>>& getStatements() const {
        return statements;
    }
    // We will iterate over these statements during Phase 6 codegen
};

#endif
