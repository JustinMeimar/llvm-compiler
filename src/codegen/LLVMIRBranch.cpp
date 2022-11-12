#include "LLVMIRBranch.h"

/* Pre Predicated Loop 
   order: (condition -> body -> merge) */
void LLVMIRBranch::createPrePredConditionalBB(const std::string& labelPrefix) {
    // llvm::Function* parentSubrt = m_builder->GetInsertBlock()->getParent(); 
    llvm::Function* parentFunc = m_builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *ConditionalBB = llvm::BasicBlock::Create(
            *m_context, "LoopCondition", parentFunc);
    llvm::BasicBlock *BodyBB = llvm::BasicBlock::Create(
            *m_context, "LoopBody", parentFunc);
    llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(
            *m_context, "LoopMerge"); 

    m_builder->CreateBr(ConditionalBB);
    m_builder->SetInsertPoint(ConditionalBB);
    blockStack.push_back(ConditionalBB);
    blockStack.push_back(BodyBB);
    blockStack.push_back(MergeBB);
}

void LLVMIRBranch::createPrePredBodyBB(llvm::Value* condition) { 
    llvm::BasicBlock *BodyBB = blockStack[blockStack.size() - 2];
    llvm::BasicBlock *MergeBB = blockStack[blockStack.size() - 1];

    m_builder->CreateCondBr(condition, BodyBB, MergeBB);
    m_builder->SetInsertPoint(BodyBB);
}

void LLVMIRBranch::createPrePredMergeBB() {
    llvm::BasicBlock *ConditionalBB = blockStack[blockStack.size() - 3];
    llvm::BasicBlock *MergeBB = blockStack[blockStack.size() - 1];

    m_builder->CreateBr(ConditionalBB);
    
    llvm::Function *parentFunc = m_builder->GetInsertBlock()->getParent();
    parentFunc->getBasicBlockList().push_back(MergeBB);
    m_builder->SetInsertPoint(MergeBB);

    blockStack.pop_back();
    blockStack.pop_back();
    blockStack.pop_back();
}

/* Post Predicated Loop 
   order: (body -> condition -> merge) */
void LLVMIRBranch::createPostPredBodyBB() {
    llvm::Function *parentFunc = m_builder->GetInsertBlock()->getParent();
    parentFunc = m_builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *BodyBB = llvm::BasicBlock::Create(
            *m_context, "PostPredicatedLoopBody", parentFunc);
    llvm::BasicBlock *ConditionalBB = llvm::BasicBlock::Create(
            *m_context, "PostPredicatedLoopCondition", parentFunc);
    llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(
            *m_context, "LoopMerge"); 

    m_builder->CreateBr(BodyBB);
    m_builder->SetInsertPoint(BodyBB);

    blockStack.push_back(BodyBB);
    blockStack.push_back(ConditionalBB);
    blockStack.push_back(MergeBB);
}

void LLVMIRBranch::createPostPredConditionalBB() {
    llvm::BasicBlock* ConditionalBB = blockStack[blockStack.size() - 2];
    m_builder->CreateBr(ConditionalBB);
    m_builder->SetInsertPoint(ConditionalBB);
}

void LLVMIRBranch::createPostPredMergeBB(llvm::Value* condition) {
    llvm::Function *parentFunc = m_builder->GetInsertBlock()->getParent();
    llvm::BasicBlock *BodyBB = blockStack[blockStack.size() - 3];
    llvm::BasicBlock *MergeBB = blockStack[blockStack.size() - 1];

    m_builder->CreateCondBr(condition, BodyBB, MergeBB);    
    parentFunc->getBasicBlockList().push_back(MergeBB);
    m_builder->SetInsertPoint(MergeBB);

    blockStack.pop_back();
    blockStack.pop_back();
    blockStack.pop_back();
}