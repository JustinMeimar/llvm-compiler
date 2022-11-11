#pragma once

#include <iostream>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/NoFolder.h"

#include <vector>

class LLVMIRBranch {
public:
    int numConditionals;
    int numPredicatedLoops;
    int numIteratorLoops;

    LLVMIRBranch(
        llvm::LLVMContext *context, 
        llvm::IRBuilder<llvm::NoFolder> *builder, 
        llvm::Module *module
    ): m_context(context), m_builder(builder), m_module(module) {};

    void createLoopConditionalBB(const std::string& labelPrefix);
    void createLoopBodyBB(llvm::Value* condition);
    void createLoopMergeBB();

private:
    // access to the context and module
    llvm::LLVMContext *m_context;
    llvm::IRBuilder<llvm::NoFolder> *m_builder;
    llvm::Module *m_module;

    std::vector<llvm::BasicBlock*> blockStack; 
};
