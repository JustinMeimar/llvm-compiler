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

    void createPrePredConditionalBB(const std::string& labelPrefix); // CreatePrePredicatedLoopEnterBasicBlock
    void createPrePredBodyBB(llvm::Value* condition);
    void createPrePredMergeBB();
    
    void createPostPredBodyBB();
    void createPostPredConditionalBB(); // CreatePostPredicatedLoopEnterBasicBlock
    void createPostPredMergeBB(llvm::Value* condition);

private:
    // access to the context and module
    llvm::LLVMContext *m_context;
    llvm::IRBuilder<llvm::NoFolder> *m_builder;
    llvm::Module *m_module;

    std::vector<llvm::BasicBlock*> blockStack; 
};
