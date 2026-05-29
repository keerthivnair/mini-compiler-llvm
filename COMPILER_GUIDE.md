# Complete Developer Guide: `T_STRING` Support, Git Operations & Multi-Platform Setup

This document serves as your ultimate guide for extending the custom compiler with dynamic string support (`T_STRING`), backing up the repository to a remote Git host (like GitHub), and setting up the entire development workspace on a new machine from scratch.

---

## 1. Adding `T_STRING` Support: Steps & Syntax

To support string literals (`"hello"`) and string variables in your programming language, we must update the entire compiler pipeline: Lexer -> Parser -> AST -> Code Generation. 

### Step 1: Lexical Analysis (`src/lexer.l`)
We need to capture string literals enclosed in double quotes. We use a regular expression, strip the quotes, allocate heap memory via `strdup()`, and return a new `T_STRING` token.

Add the following rule to `src/lexer.l` inside the `%%` rules section:

```lex
\"([^"\\]|\\.)*\" {
    /* 
     * String literal matched!
     * 1. Get length and allocate new buffer.
     * 2. Strip opening and closing double quotes.
     * 3. Terminate with null character and store in yylval.str_val.
     */
    int len = strlen(yytext);
    char* str = (char*)malloc(len - 1);
    strncpy(str, yytext + 1, len - 2);
    str[len - 2] = '\0';
    yylval.str_val = str;
    return T_STRING;
}
```

### Step 2: Syntax Analysis (`src/parser.y`)
We must declare `T_STRING` as a token that holds a string value and add it as an expression rule.

1. **Declare the token** in `src/parser.y` (e.g., alongside `T_IDENTIFIER`):
   ```yacc
   %token <str_val> T_STRING
   ```

2. **Add string literals to expressions** (`expr` block):
   ```yacc
   expr:
       T_INT {
           $$ = new NumberAST($1);
       }
       | T_STRING {
           /* Construct StringAST node and free temporary scanner memory */
           $$ = new StringAST($1);
           free($1);
       }
       | T_IDENTIFIER {
           $$ = new VariableAST($1);
           free($1);
       }
       ...
   ```

### Step 3: Abstract Syntax Tree (`src/ast.h`)
We must define a new `StringAST` class representing a string literal in the AST.

Add the `StringAST` definition right next to `NumberAST` inside `src/ast.h`:

```cpp
class StringAST : public ASTNode {
    std::string val;
public:
    StringAST(const std::string& val) : val(val) {}
    const std::string& getValue() const { return val; }
    
    // Core IR Code Generator implementation
    llvm::Value* codegen(CodeGenContext& context) override;
};
```

### Step 4: Code Generation (`src/codegen.cpp`)
We must implement string constant creation, dynamic stack allocation for variable assignments, variable loading, and format switching during prints.

1. **Implement `StringAST::codegen`** at the top of `src/codegen.cpp`:
   ```cpp
   Value* StringAST::codegen(CodeGenContext& context) {
       // Returns a pointer to a global string constant (char* equivalent in LLVM)
       return context.builder->CreateGlobalStringPtr(val);
   }
   ```

2. **Enhance `AssignmentAST::codegen`** to dynamically allocate stack space based on the type of the value being stored (instead of hardcoding `i32`):
   ```cpp
   Value* AssignmentAST::codegen(CodeGenContext& context) {
       Value* val = expr->codegen(context);
       if (!val) return nullptr;

       Value* variable = context.symTable.lookup(name);
       if (!variable) {
           // DYNAMIC ALLOCATION: Allocate stack slot using the computed value's exact type!
           variable = context.builder->CreateAlloca(val->getType(), nullptr, name.c_str());
           context.symTable.define(name, variable);
       }

       context.builder->CreateStore(val, variable);
       return val;
   }
   ```

3. **Enhance `VariableAST::codegen`** to dynamically extract the allocated type from the alloca slot to load values correctly:
   ```cpp
   Value* VariableAST::codegen(CodeGenContext& context) {
       Value* ptr = context.symTable.lookup(name);
       if (!ptr) {
           std::cerr << "Unknown variable name: " << name << std::endl;
           return nullptr;
       }
       // Query the exact type of variables dynamically from their Stack Slot (AllocaInst)
       llvm::Type* loadedType = Type::getInt32Ty(*context.llvmContext);
       if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(ptr)) {
           loadedType = alloca->getAllocatedType();
       }
       return context.builder->CreateLoad(loadedType, ptr, name.c_str());
   }
   ```

4. **Enhance `PrintAST::codegen`** to automatically switch the print formatter between integers (`%d\n`) and strings (`%s\n`):
   ```cpp
   Value* PrintAST::codegen(CodeGenContext& context) {
       Value* val = expr->codegen(context);
       if (!val) return nullptr;

       std::vector<Type*> printfArgs;
       printfArgs.push_back(Type::getInt8PtrTy(*context.llvmContext)); // char*
       FunctionType* printfType = FunctionType::get(Type::getInt32Ty(*context.llvmContext), printfArgs, true);
       FunctionCallee printfFunc = context.module->getOrInsertFunction("printf", printfType);
       
       // Detect if this is a pointer (string pointer) or an integer
       Value* formatStr;
       if (val->getType()->isPointerTy()) {
           formatStr = context.builder->CreateGlobalStringPtr("%s\n");
       } else {
           formatStr = context.builder->CreateGlobalStringPtr("%d\n");
       }
       
       std::vector<Value*> args;
       args.push_back(formatStr);
       args.push_back(val);
       
       return context.builder->CreateCall(printfFunc, args, "printfcall");
   }
   ```

---

## 2. Pushing the Mini-Compiler to a Remote Git Host (GitHub/GitLab)

Your local repository is initialized and your work is safely committed on the local `master` branch. Follow these steps to push it to a remote server:

### Step 1: Create a Repository on GitHub/GitLab
- Go to GitHub, click **New Repository**.
- Name it (e.g., `mini-compiler-llvm`).
- **CRITICAL:** Do NOT check "Add a README", ".gitignore", or "license" (we have already created these locally; checking them will cause branch merge conflicts).

### Step 2: Push via Terminal
Run the following commands inside your local project directory `/home/ubuntu/nvidia/MINI_COMPILER_PROJECT`:

```bash
# Rename the default branch to 'main'
git branch -M main

# Add the remote GitHub repository URL (Replace with your actual GitHub URL!)
git remote add origin https://github.com/YOUR_USERNAME/mini-compiler-llvm.git

# Push your code to the remote repository
git push -u origin main
```

---

## 3. Setting Up on a New Machine: Cross-Platform Instructions

To run, build, and test this project on a clean computer, you need basic build tools, a C++17 compiler, Flex, Bison, and LLVM packages.

### Option A: Ubuntu / Debian (Linux)
Run these commands in your shell:
```bash
# Update package repositories
sudo apt-get update

# Install GCC/G++, CMake, Flex, Bison, and LLVM
sudo apt-get install -y build-essential cmake flex bison llvm clang libllvm-dev
```

### Option B: macOS
Make sure you have [Homebrew](https://brew.sh/) installed, then run:
```bash
# Install CMake, Flex, Bison, and LLVM
brew install cmake flex bison llvm

# Note: macOS defaults to the built-in system Bison/Flex which can be outdated. 
# Homebrew automatically displays path extension instructions. Run these if required:
export PATH="/opt/homebrew/opt/bison/bin:/opt/homebrew/opt/flex/bin:$PATH"
```

### Option C: Arch Linux
```bash
sudo pacman -Syu
sudo pacman -S base-devel cmake flex bison llvm clang
```

### Option D: Windows
Compiler development on Windows is highly robust through three primary developer-grade methods:

#### Method 1: WSL2 (Windows Subsystem for Linux - Highly Recommended)
Using WSL2 allows you to run a native Linux environment inside Windows with zero performance overhead. This is the cleanest and most industry-standard way to write LLVM compilers on Windows.
1. Open **PowerShell** or **Command Prompt** as Administrator and run:
   ```powershell
   wsl --install
   ```
2. Restart your computer. Once restarted, launch the installed **Ubuntu** app from your Start menu.
3. Run the standard Linux setup commands:
   ```bash
   sudo apt-get update
   sudo apt-get install -y build-essential cmake flex bison llvm clang libllvm-dev git
   ```

#### Method 2: MSYS2 (MinGW-w64 - For Native Windows `.exe` Compilation)
MSYS2 provides modern C++ toolchains and native package managers for Windows.
1. Download and run the installer from the official [MSYS2 Website](https://www.msys2.org/).
2. Open the **MSYS2 UCRT64** terminal from your Start Menu.
3. Install the compiler toolchain, Flex, Bison, CMake, Git, and LLVM packages:
   ```bash
   pacman -S --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-flex mingw-w64-ucrt-x86_64-bison mingw-w64-ucrt-x86_64-llvm mingw-w64-ucrt-x86_64-clang git
   ```
4. **Environment Variables:** Add `C:\msys64\ucrt64\bin` to your Windows System environment variable `PATH` so your IDE (like VS Code) can access the tools.
5. In your command prompt, configure and compile with CMake using MinGW:
   ```cmd
   mkdir build
   cd build
   cmake -G "MinGW Makefiles" ..
   mingw32-make
   ```

#### Method 3: Visual Studio (MSVC) + WinFlexBison (For Microsoft C++ Toolchain)
For native development within Visual Studio:
1. Install **Visual Studio** (Community/Professional) with the **"Desktop development with C++"** workload selected.
2. Install **Chocolatey** (Windows Package Manager) by opening an Administrative PowerShell and following the [Chocolatey installation instructions](https://chocolatey.org/install).
3. Install the dependencies:
   ```powershell
   choco install -y winflexbison3 llvm cmake git
   ```
4. Open the **Developer Command Prompt for VS** (from your Start Menu) and compile your project:
   ```cmd
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

---

## 4. Compiling and Running the Compiler
Once libraries are installed on the new machine:

```bash
# 1. Clone the repository
git clone <your-git-repo-url>
cd MINI_COMPILER_PROJECT

# 2. Create the build directory
mkdir build && cd build

# 3. Configure the build system (cmake)
cmake ..

# 4. Compile the project
make

# 5. Run the compiler with sample code!
./minicompiler ../examples/sample.mc
```

---
> [!TIP]
> **Pro-Tip for Interviews:** LLVM uses static single assignment (SSA). If your variable is updated, it creates a new virtual register. Using stack memory allocation via `alloca` lets the compiler treat variables as memory slots and delegating optimization to LLVM's `mem2reg` pass, which promotes memory accesses to high-performance SSA registers automatically!
