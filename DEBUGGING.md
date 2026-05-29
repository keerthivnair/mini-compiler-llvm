# Debugging Guide

This document tracks debugging strategies for building the compiler.

## Phase 1: Setup & Build Debugging

**Issue:** CMake cannot find LLVM.
**Reason:** LLVM doesn't use standard pkg-config universally, it often uses `llvm-config` or `LLVMConfig.cmake`.
**Solution:** Ensure `find_package(LLVM REQUIRED CONFIG)` is used and LLVM paths are available in `PATH` or `LLVM_DIR` is set to point to `/usr/lib/llvm-XX/cmake`.

**Issue:** Flex and Bison undefined references (`undefined reference to yywrap`).
**Reason:** Flex by default expects you to provide a function called `yywrap` to indicate what to do when the end of a file is reached.
**Solution:** Add `%option noyywrap` to the lexer file to tell Flex we are only scanning one file, so it doesn't need to ask for more.
