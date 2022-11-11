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

#include "SubroutineSymbol.h"
#include "LLVMIRFunction.h"


namespace gazprea {

class LLVMGen {
    public: 
        llvm::LLVMContext globalCtx;
        llvm::IRBuilder<llvm::NoFolder> ir;
        llvm::Module mod;
        std::string outfile;

        llvm::StructType *runtimeTypeTy;
        llvm::StructType *runtimeVariableTy;
        llvm::Function* currentSubroutine;

        LLVMIRFunction llvmFunction;

        LLVMGen(std::shared_ptr<SymbolTable> symtab, std::string& outfile);
        ~LLVMGen();

        //AST Walker
        void visit(std::shared_ptr<AST> t);
        void visitChildren(std::shared_ptr<AST> t);
        void visitSubroutineDeclDef(std::shared_ptr<AST> t); 
        void visitReturn(std::shared_ptr<AST> t);
        
        // Expression Atom
        void visitBooleanAtom(std::shared_ptr<AST> t);
        void visitCharacterAtom(std::shared_ptr<AST> t);
        void visitIntegerAtom(std::shared_ptr<AST> t);
        void visitRealAtom(std::shared_ptr<AST> t);
        void visitIdentityAtom(std::shared_ptr<AST> t);
        void visitNullAtom(std::shared_ptr<AST> t);
        void visitStringLiteral(std::shared_ptr<AST> t);

        // Other Sub-Expression rules
        void visitExpression(std::shared_ptr<AST> t);
        void visitCast(std::shared_ptr<AST> t);

        // Stream Statements
        void visitInputStreamStatement(std::shared_ptr<AST> t);
        void visitOutputStreamStatement(std::shared_ptr<AST> t);

        //Helper Methods 
        void Print();
};

}