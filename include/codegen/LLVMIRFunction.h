#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/NoFolder.h"
#include <map>
#include "GazpreaParser.h"

/**
 * Manages function declaration
 * Provide wrappers for function calls
 */
class LLVMIRFunction {
public:
    LLVMIRFunction(
        llvm::LLVMContext *context, 
        llvm::IRBuilder<llvm::NoFolder> *builder, 
        llvm::Module *module
    ): m_context(context), m_builder(builder), m_module(module) {};

    // should be called at the start of LLVM IR generation to declare external functions
    void declareAllFunctions();

    llvm::Function *getFunctionFromName(const std::string& name);
    llvm::FunctionType *getFTyFromName(const std::string& name);
    llvm::Value *call(const std::string& funcName, llvm::ArrayRef<llvm::Value *> args);
    llvm::StructType *runtimeTypeTy;
    llvm::StructType *runtimeVariableTy;

private:
    // access to the context and module
    llvm::LLVMContext *m_context;
    llvm::IRBuilder<llvm::NoFolder> *m_builder;
    llvm::Module *m_module;

    std::map<std::string, llvm::Function *> m_nameToFunction;  // used for function calls
    std::map<std::string, llvm::FunctionType *> m_nameToFTy;  // used for function calls

    /**
     * declare a function and add its name to the map
     * @param fTy function signature
     * @param name name of the function
     * @return function created
     */
    llvm::Function *declareFunction(llvm::FunctionType *fTy, const std::string& name);
};
