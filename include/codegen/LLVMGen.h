#pragma once
#include "AST.h"
#include "SymbolTable.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/IR/Verifier.h" 

#include "llvm/Support/raw_os_ostream.h"

#include "LLVMTypes.h"


namespace gazprea {

class LLVMGen {
    private:
        LLVMTypes types;

    public: 
        llvm::LLVMContext globalCtx;
        llvm::IRBuilder<llvm::NoFolder> ir;
        llvm::Module mod;
        std::string outfile;


        llvm::Value* currentSubroutine;

        LLVMGen(std::shared_ptr<SymbolTable> symtab, std::string& outfile);
        ~LLVMGen();

        //AST Walker
        void visit(std::shared_ptr<AST> t);
        void visitChildren(std::shared_ptr<AST> t);
        void visitSubroutineDeclDef(std::shared_ptr<AST> t); 
        void visitReturn(std::shared_ptr<AST> t);

        //Helper Methods 
        void Print();
};

}