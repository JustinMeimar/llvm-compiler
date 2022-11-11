#include "LLVMIRFunction.h"
#include <iostream>

llvm::Function *LLVMIRFunction::getFunctionFromName(const std::string& name) {
    return m_nameToFunction[name];
}

llvm::FunctionType *LLVMIRFunction::getFTyFromName(const std::string& name) {
    return m_nameToFTy[name];
}

llvm::Value *LLVMIRFunction::call(const std::string& funcName, llvm::ArrayRef<llvm::Value *> args) {
    return m_builder->CreateCall(m_nameToFTy[funcName], m_nameToFunction[funcName], args);
}

void LLVMIRFunction::declareAllFunctions() {
    llvm::Type * voidTy = m_builder->getVoidTy();
    llvm::IntegerType * int1Ty = m_builder->getInt1Ty();
    llvm::IntegerType * int8Ty = m_builder->getInt8Ty();
    llvm::IntegerType * int32Ty = m_builder->getInt32Ty();
    // llvm::IntegerType * int64Ty = m_builder->getInt64Ty();
    llvm::Type * floatTy = m_builder->getFloatTy();

    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo(), int1Ty}, false),
        "variableInitFromBooleanScalar"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo(), int32Ty}, false),
        "variableInitFromIntegerScalar"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo(), floatTy}, false),
        "variableInitFromRealScalar"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo(), int8Ty}, false),
        "variableInitFromCharacterScalar"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo()}, false),
        "variableInitFromNullScalar"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo()}, false),
        "variableInitFromIdentityScalar"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo()}, false),
        "variableReadFromStdin"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo()}, false),
        "variablePrintToStdout"
    );
    declareFunction(
        llvm::FunctionType::get(runtimeVariableTy->getPointerTo(), false),
        "variableMalloc"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo(), runtimeTypeTy->getPointerTo(), runtimeVariableTy->getPointerTo()}, false),
        "variableInitFromCast"
    );
}

llvm::Function *LLVMIRFunction::declareFunction(llvm::FunctionType *fTy, const std::string &name) {
    if (m_nameToFunction.count(name) != 0) {
        std::cerr << "Error! Attempt to declare function with the same name the second time!\n";
    }

    auto * func = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(name, fTy).getCallee());
    m_nameToFunction[name] = func;
    m_nameToFTy[name] = fTy;

    return func;
}