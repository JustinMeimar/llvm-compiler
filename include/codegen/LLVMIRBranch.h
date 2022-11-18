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

    bool hitReturnStat = true;
     
    std::vector<llvm::BasicBlock*> blockStack; 
    std::vector<llvm::BasicBlock*> loopBlockStack; // keep track of which loop to break out of

    LLVMIRBranch(
        llvm::LLVMContext *context, 
        llvm::IRBuilder<llvm::NoFolder> *builder, 
        llvm::Module *module
    ): m_context(context), m_builder(builder), m_module(module) {};

    //Conditional Control Flow
    void createConditionalBasicBlock(llvm::Value* condition, llvm::BasicBlock* merge);
    // void createConditionalElseIf(llvm::Value* condition, llvm::BasicBlock* commonMerge);
    // void createConditionalElse();

    //Pre Predicated Loop
    void createPrePredConditionalBB(const std::string& labelPrefix); // Entry point
    void createPrePredBodyBB(llvm::Value* condition);
    void createPrePredMergeBB();

    //Post Predicated Loop 
    void createPostPredBodyBB(); //Entry point is body
    void createPostPredConditionalBB(); 
    void createPostPredMergeBB(llvm::Value* condition);

private:
    // access to the context and module
    llvm::LLVMContext *m_context;
    llvm::IRBuilder<llvm::NoFolder> *m_builder;
    llvm::Module *m_module;

};
