#pragma once

#include <string>
#include <memory>
#include <vector>

#include "Symbol.h"
#include "Type.h"
#include "LocalScope.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Function.h"

namespace gazprea {
    class SubroutineSymbol : public Symbol, public Scope {
    private:
        std::shared_ptr<Scope> enclosingScope;
    public:
        bool isProcedure;  // True if the subroutine is a procedure, False if the subroutine is a function
        bool isBuiltIn;  // True if this subroutine is built-in, false otherwise
        bool hasReturn; 
        std::shared_ptr<AST> declaration;
        std::shared_ptr<AST> definition;
        llvm::Value* stackPtr = nullptr; 
        int numTimesDeclare = 0;  // If forward declaration, this value is 2, otherwise 1
        int numTimesDefined = 0;  // If 0 then undecalred subroutine error
        std::vector<std::shared_ptr<Symbol>> orderedArgs;
        llvm::Function *llvmFunction;
        std::shared_ptr<LocalScope> subroutineDirectChildScope;
        std::vector<llvm::Value *> oldParameterTypes;
        SubroutineSymbol(std::string name, std::shared_ptr<Type> retType, std::shared_ptr<Scope> enclosingScope, bool isProcedure, bool isBuiltIn);
        std::shared_ptr<Symbol> resolve(const std::string &name);
        void define(std::shared_ptr<Symbol> sym);
        std::shared_ptr<Scope> getEnclosingScope();
        std::string getScopeName();
        std::string toString();
        bool isType() {
            return false;
        }
    };
}


