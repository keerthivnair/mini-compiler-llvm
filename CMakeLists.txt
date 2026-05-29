cmake_minimum_required(VERSION 3.14)
project(MiniCompiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Flex and Bison
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set LLVM include and lib paths
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Map LLVM components
llvm_map_components_to_libnames(llvm_libs Core ExecutionEngine JITLink OrcJIT MCJIT Support native IRReader)

# Flex and Bison targets
# We output to the CMAKE_CURRENT_BINARY_DIR so we don't clutter the src tree
BISON_TARGET(Parser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)
FLEX_TARGET(Scanner src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
ADD_FLEX_BISON_DEPENDENCY(Scanner Parser)

# Add include directory for generated headers
include_directories(${CMAKE_CURRENT_BINARY_DIR} src)

# The executable
add_executable(minicompiler
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Scanner_OUTPUTS}
    src/main.cpp
    src/ast.cpp
    src/symbol_table.cpp
    src/codegen.cpp
    src/jit.cpp
)

# Link against LLVM
target_link_libraries(minicompiler ${llvm_libs})

# Enable strict warnings for good C++ practices
target_compile_options(minicompiler PRIVATE -Wall -Wextra -pedantic)
