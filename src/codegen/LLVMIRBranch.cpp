#include "LLVMIRBranch.h"

void LLVMIRBranch::createLoopConditionalBB(const std::string& labelPrefix) {
    //
    llvm::Function* parentSubrt = m_builder->GetInsertBlock()->getParent(); 
    llvm::BasicBlock *ConditionalBB = llvm::BasicBlock::Create(
            *m_context, "LoopCondition", parentSubrt);
    llvm::BasicBlock *BodyBB = llvm::BasicBlock::Create(
            *m_context, "LoopBody", parentSubrt);
    llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(
            *m_context, "LoopMerge"); 

    m_builder->SetInsertPoint(ConditionalBB);

    blockStack.push_back(ConditionalBB);
    blockStack.push_back(BodyBB);
    blockStack.push_back(MergeBB);
}

void LLVMIRBranch::createLoopBodyBB(llvm::Value* condition) {

    // std::cout << blockStack[blockStack.size() - 2];
    // std::cout << blockStack[blockStack.size() - 1];
    llvm::BasicBlock *BodyBB = blockStack[blockStack.size() - 2];
    llvm::BasicBlock *MergeBB = blockStack[blockStack.size() - 1];

    m_builder->CreateCondBr(condition, BodyBB, MergeBB);
    m_builder->SetInsertPoint(BodyBB);
    //evaluate condition
    
}

void LLVMIRBranch::createLoopMergeBB() {
    llvm::BasicBlock *ConditionalBB = blockStack[blockStack.size() - 2];
    llvm::BasicBlock *BodyBB = blockStack[blockStack.size() - 1];

    
}

