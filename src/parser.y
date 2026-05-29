%{
#include "common.h"
#include "ast.h"
#include <iostream>

extern int yylex();
extern void yyerror(const char* s);

// Global pointer to hold the root of our program once parsing is done
ProgramAST* programRoot;

%}

/* Define the semantic values that tokens and grammar rules can hold */
%union {
    int int_val;
    char* str_val;
    ASTNode* ast_node;
    std::vector<ASTNode*>* stmt_list;
    std::vector<std::string>* str_list;
    std::vector<ASTNode*>* expr_list;
}

/* Tokens from Lexer */
%token <int_val> T_INT
%token <str_val> T_IDENTIFIER
%token T_FUNC T_RETURN T_IF T_WHILE T_PRINT
%token T_PLUS T_MINUS T_MUL T_DIV T_MOD
%token T_LESS T_GREATER T_EQUALS T_ASSIGN
%token T_LPAREN T_RPAREN T_LBRACE T_RBRACE T_COMMA T_SEMICOLON

/* Define types for our non-terminal grammar rules */
%type <ast_node> expr stmt func_decl
%type <stmt_list> stmt_list
%type <str_list> param_list
%type <expr_list> arg_list

/* Operator Precedence (Solves Shift/Reduce conflicts!) */
%left T_LESS T_GREATER T_EQUALS 
%left T_PLUS T_MINUS
%left T_MUL T_DIV T_MOD

%%

program:
    stmt_list {
        programRoot = new ProgramAST();
        for (ASTNode* stmt : *$1) {
            // We convert the raw pointers from Bison into unique_ptrs
            programRoot->addStatement(std::unique_ptr<ASTNode>(stmt));
        }
        delete $1; // Free the temporary vector
    }
    ;

stmt_list:
    stmt {
        $$ = new std::vector<ASTNode*>();
        $$->push_back($1);
    }
    | stmt_list stmt {
        $1->push_back($2);
        $$ = $1;
    }
    ;

stmt:
    T_IDENTIFIER T_ASSIGN expr T_SEMICOLON {
        $$ = new AssignmentAST($1, std::unique_ptr<ASTNode>($3));
        free($1); // Free the strdup memory from the lexer!
    }
    | T_RETURN expr T_SEMICOLON {
        $$ = new ReturnAST(std::unique_ptr<ASTNode>($2));
    }
    | T_PRINT T_LPAREN expr T_RPAREN T_SEMICOLON {
        $$ = new PrintAST(std::unique_ptr<ASTNode>($3));
    }
    | T_IF T_LPAREN expr T_RPAREN T_LBRACE stmt_list T_RBRACE {
        // Convert raw vector to unique_ptrs
        std::vector<std::unique_ptr<ASTNode>> body;
        for (auto n : *$6) body.push_back(std::unique_ptr<ASTNode>(n));
        delete $6;
        $$ = new IfAST(std::unique_ptr<ASTNode>($3), std::move(body));
    }
    | expr T_SEMICOLON {
        $$ = $1; // For things like function calls that don't assign
    }
    | func_decl {
        $$ = $1;
    }
    ;

func_decl:
    T_FUNC T_IDENTIFIER T_LPAREN param_list T_RPAREN T_LBRACE stmt_list T_RBRACE {
        std::vector<std::unique_ptr<ASTNode>> body;
        for (auto n : *$7) body.push_back(std::unique_ptr<ASTNode>(n));
        delete $7;
        
        $$ = new FunctionAST($2, *$4, std::move(body));
        free($2);
        delete $4;
    }
    ;

param_list:
    /* empty */ { $$ = new std::vector<std::string>(); }
    | T_IDENTIFIER {
        $$ = new std::vector<std::string>();
        $$->push_back($1);
        free($1);
    }
    | param_list T_COMMA T_IDENTIFIER {
        $1->push_back($3);
        $$ = $1;
        free($3);
    }
    ;

expr:
    T_INT {
        $$ = new NumberAST($1);
    }
    | T_IDENTIFIER {
        $$ = new VariableAST($1);
        free($1);
    }
    | expr T_PLUS expr   { 
        auto* l = dynamic_cast<NumberAST*>($1);
        auto* r = dynamic_cast<NumberAST*>($3);
        if (l && r) {
            $$ = new NumberAST(l->getIntValue() + r->getIntValue());
            delete $1; 
            delete $3;
        } else {
            $$ = new BinaryExprAST('+', std::unique_ptr<ASTNode>($1), std::unique_ptr<ASTNode>($3)); 
        }
        }
    | expr T_MINUS expr  { $$ = new BinaryExprAST('-', std::unique_ptr<ASTNode>($1), std::unique_ptr<ASTNode>($3)); }
    | expr T_MUL expr    { $$ = new BinaryExprAST('*', std::unique_ptr<ASTNode>($1), std::unique_ptr<ASTNode>($3)); }
    | expr T_DIV expr    { $$ = new BinaryExprAST('/', std::unique_ptr<ASTNode>($1), std::unique_ptr<ASTNode>($3)); }
    | expr T_LESS expr   { $$ = new BinaryExprAST('<', std::unique_ptr<ASTNode>($1), std::unique_ptr<ASTNode>($3)); }
    | expr T_MOD expr    { $$ = new BinaryExprAST('%', std::unique_ptr<ASTNode>($1), std::unique_ptr<ASTNode>($3)); }
    | expr T_GREATER expr { $$ = new BinaryExprAST('>', std::unique_ptr<ASTNode>($1), std::unique_ptr<ASTNode>($3)); }
    | expr T_EQUALS expr { $$ = new BinaryExprAST('=', std::unique_ptr<ASTNode>($1), std::unique_ptr<ASTNode>($3)); }
    | T_IDENTIFIER T_LPAREN arg_list T_RPAREN {
        std::vector<std::unique_ptr<ASTNode>> args;
        for (auto n : *$3) args.push_back(std::unique_ptr<ASTNode>(n));
        delete $3;
        
        $$ = new FunctionCallAST($1, std::move(args));
        free($1);
    }
    ;

arg_list:
    /* empty */ { $$ = new std::vector<ASTNode*>(); }
    | expr {
        $$ = new std::vector<ASTNode*>();
        $$->push_back($1);
    }
    | arg_list T_COMMA expr {
        $1->push_back($3);
        $$ = $1;
    }
    ;

%%
