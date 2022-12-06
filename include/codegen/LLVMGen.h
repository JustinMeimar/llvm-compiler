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

#include "SubroutineSymbol.h"
#include "VariableSymbol.h"
#include "LLVMIRBranch.h"
#include "LLVMIRFunction.h"

#include "MatrixType.h"
#include "TypedefTypeSymbol.h"
#include "LocalScope.h"
#include "TypePromote.h"

namespace gazprea {

class LLVMGen {
    public:
        std::shared_ptr<SymbolTable> symtab;
        llvm::LLVMContext globalCtx;
        llvm::IRBuilder<llvm::NoFolder> ir;
        llvm::Module mod;
        std::string outfile;

        llvm::StructType *runtimeTypeTy;
        llvm::StructType *runtimeVariableTy;
        llvm::StructType *runtimeStackTy;
        llvm::StructType *runtimeStackItemTy;
        
        llvm::Value* globalStack; //initialized in initializeGlobalVariables 

        llvm::Function* currentSubroutine;

        LLVMIRFunction llvmFunction;
        LLVMIRBranch llvmBranch;
        int numExprAncestors;
        std::shared_ptr<TypePromote> tp;

        LLVMGen(std::shared_ptr<SymbolTable> symtab, std::shared_ptr<TypePromote> tp, std::string& outfile);
        ~LLVMGen();

        //AST Walker
        void visit(std::shared_ptr<AST> t);
        void visitChildren(std::shared_ptr<AST> t);
        void visitSubroutineDeclDef(std::shared_ptr<AST> t); 
        void visitReturn(std::shared_ptr<AST> t);

        // Control Flow
        void visitConditionalStatement(std::shared_ptr<AST> t);
        void viistInfiniteLoop(std::shared_ptr<AST> t);
        void visitPrePredicatedLoop(std::shared_ptr<AST> t);
        void visitPostPredicatedLoop(std::shared_ptr<AST> t);
        void visitIteratorLoop(std::shared_ptr<AST> t);
        void visitBlock(std::shared_ptr<AST> t);

        // Expression Atom
        void visitBooleanAtom(std::shared_ptr<AST> t);
        void visitCharacterAtom(std::shared_ptr<AST> t);
        void visitIntegerAtom(std::shared_ptr<AST> t);
        void visitRealAtom(std::shared_ptr<AST> t);
        void visitIdentityAtom(std::shared_ptr<AST> t);
        void visitNullAtom(std::shared_ptr<AST> t);
        void visitStringLiteral(std::shared_ptr<AST> t);
        void visitIdentifier(std::shared_ptr<AST> t);
        void visitGenerator(std::shared_ptr<AST> t);
        void visitFilter(std::shared_ptr<AST> t);
        void visitTupleLiteral(std::shared_ptr<AST> t);
        void visitVectorMatrixLiteral(std::shared_ptr<AST> t);

        // Other Sub-Expression rules
        void visitExpression(std::shared_ptr<AST> t);
        void visitCast(std::shared_ptr<AST> t);
        void visitTypedef(std::shared_ptr<AST> t);

        // Stream Statements
        void visitInputStreamStatement(std::shared_ptr<AST> t);
        void visitOutputStreamStatement(std::shared_ptr<AST> t);
        void visitCallSubroutineStatement(std::shared_ptr<AST> t);

        // Operations
        void visitBinaryOperation(std::shared_ptr<AST> t);
        void visitUnaryOperation(std::shared_ptr<AST> t);
        void visitIndexing(std::shared_ptr<AST> t);
        void visitInterval(std::shared_ptr<AST> t);
        void visitStringConcatenation(std::shared_ptr<AST> t);

        // Call
        void visitCallSubroutineInExpression(std::shared_ptr<AST> t);

        // Other Statements
        void visitVarDeclarationStatement(std::shared_ptr<AST> t);
        void visitAssignmentStatement(std::shared_ptr<AST> t);
        void visitParameterAtom(std::shared_ptr<AST> t);
        void visitBreak(std::shared_ptr<AST> t);
        void visitContinue(std::shared_ptr<AST> t);

        // Type
        void visitUnqualifiedType(std::shared_ptr<AST> t);

        // Tuple
        void visitTupleAccess(std::shared_ptr<AST> t);

        //Helper Methods 
        void Print();
        void initializeGlobalVariables();
        void freeGlobalVariables();
        void freeAllVariablesDeclaredInBlockScope(std::shared_ptr<LocalScope> scope);
        void initializeSubroutineParameters(std::shared_ptr<SubroutineSymbol> subroutineSymbol);
        void freeSubroutineParameters(std::shared_ptr<SubroutineSymbol> subroutineSymbol);
        void freeExpressionIfNecessary(std::shared_ptr<AST> t);
        void freeExprAtomIfNecessary(std::shared_ptr<AST> t);
        llvm::Value* getStack();
        std::string unescapeString(const std::string &s);

        //Iterator loop Generator & Filter Helper Methods
        llvm::Value* createBranchCondition(llvm::Value* currentIndex, llvm::Value* domainLength);
        void initializeVariableSymbol(std::shared_ptr<AST> t, llvm::Value* domainVariable);
        void incrementIndex(llvm::Value* index, unsigned int increment);
        void initializeDomainVariable(llvm::Value* domainVariable, llvm::Value* domainArray, llvm::Value* index);
};

}