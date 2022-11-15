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
    // llvm::IntegerType * int1Ty = m_builder->getInt1Ty();
    llvm::IntegerType * int8Ty = m_builder->getInt8Ty();
    llvm::IntegerType * int32Ty = m_builder->getInt32Ty();
    llvm::IntegerType * int64Ty = m_builder->getInt64Ty();
    llvm::Type * floatTy = m_builder->getFloatTy();

    // Expression Atom
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo(), int32Ty}, false),
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
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo(), int64Ty, int8Ty->getPointerTo() }, false),
        "variableInitFromString"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo()}, false),
        "variableInitFromNullScalar"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo()}, false),
        "variableInitFromIdentityScalar"
    );
    
    // Other Expression
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo(), runtimeTypeTy->getPointerTo(), runtimeVariableTy->getPointerTo() }, false),
        "variableInitFromCast"
    );
    
    // Stream Statement
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo()}, false),
        "variableReadFromStdin"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo()}, false),
        "variablePrintToStdout"
    );

    // Operations
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo(), runtimeVariableTy->getPointerTo(), int32Ty}, false),
        "variableInitFromUnaryOp"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, {runtimeVariableTy->getPointerTo(), runtimeVariableTy->getPointerTo(), runtimeVariableTy->getPointerTo(), int32Ty}, false),
        "variableInitFromBinaryOp"
    );

    // Other
    declareFunction(
        llvm::FunctionType::get(runtimeVariableTy->getPointerTo(), false),
        "variableMalloc"
    );

    declareFunction(
        llvm::FunctionType::get(runtimeTypeTy->getPointerTo(), false),
        "typeMalloc"
    );

    // Var Declaration and Assignment
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeVariableTy->getPointerTo(), runtimeTypeTy->getPointerTo(), runtimeVariableTy->getPointerTo() }, false),
        "variableInitFromDeclaration"
    );

    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeVariableTy->getPointerTo(), runtimeVariableTy->getPointerTo() }, false),
        "variableAssignment"
    );


    declareFunction(
        llvm::FunctionType::get(int32Ty, { runtimeVariableTy->getPointerTo() }, false),
        "variableGetIntegerValue"
    );
    declareFunction(
        llvm::FunctionType::get(int32Ty, { runtimeVariableTy->getPointerTo() }, false),
        "variableGetBooleanValue"
    );

    // TypeInit
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeTypeTy->getPointerTo() }, false),
        "typeInitFromBooleanScalar"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeTypeTy->getPointerTo() }, false),
        "typeInitFromIntegerScalar"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeTypeTy->getPointerTo() }, false),
        "typeInitFromRealScalar"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeTypeTy->getPointerTo() }, false),
        "typeInitFromCharacterScalar"
    );
    
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeTypeTy->getPointerTo() }, false),
        "typeInitFromIntegerInterval"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeTypeTy->getPointerTo(), runtimeVariableTy->getPointerTo(), runtimeTypeTy->getPointerTo() }, false),
        "typeInitFromVectorSizeSpecification"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeTypeTy->getPointerTo(), runtimeVariableTy->getPointerTo(), runtimeVariableTy->getPointerTo(), runtimeTypeTy->getPointerTo() }, false),
        "typeInitFromMatrixSizeSpecification"
    );
    
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeTypeTy->getPointerTo() }, false),
        "typeInitFromUnknownType"
    );

    // Other
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeVariableTy->getPointerTo(), runtimeTypeTy->getPointerTo(), runtimeVariableTy->getPointerTo() }, false),
        "variableInitFromParameter"
    );

    // Copy Variable
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeVariableTy->getPointerTo(), runtimeVariableTy->getPointerTo() }, false),
        "variableInitFromMemcpy"
    );
    // Free
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeVariableTy->getPointerTo() }, false),
        "variableDestructThenFree"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeTypeTy->getPointerTo() }, false),
        "typeDestructThenFree"
    );

    // Tuple
    declareFunction(
        llvm::FunctionType::get(runtimeVariableTy->getPointerTo()->getPointerTo(), { int64Ty }, false),
        "variableArrayMalloc"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeVariableTy->getPointerTo()->getPointerTo(), int64Ty, runtimeVariableTy->getPointerTo() }, false),
        "variableArraySet"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeVariableTy->getPointerTo()->getPointerTo() }, false),
        "variableArrayFree"
    );
    declareFunction(
        llvm::FunctionType::get(runtimeTypeTy->getPointerTo()->getPointerTo(), { int64Ty }, false),
        "typeArrayMalloc"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeTypeTy->getPointerTo()->getPointerTo(), int64Ty, runtimeTypeTy->getPointerTo() }, false),
        "typeArraySet"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeTypeTy->getPointerTo()->getPointerTo() }, false),
        "typeArrayFree"
    );

    declareFunction(
        llvm::FunctionType::get(int64Ty->getPointerTo(), { int64Ty }, false),
        "stridArrayMalloc"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, { int64Ty->getPointerTo(), int64Ty, int64Ty }, false),
        "stridArraySet"
    );
    declareFunction(
        llvm::FunctionType::get(voidTy, { int64Ty->getPointerTo() }, false),
        "stridArrayFree"
    );


    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeTypeTy->getPointerTo(), int64Ty, runtimeTypeTy->getPointerTo()->getPointerTo(), int64Ty->getPointerTo() }, false),
        "typeInitFromTupleType"
    );

    declareFunction(
        llvm::FunctionType::get(voidTy, { runtimeVariableTy->getPointerTo(), int64Ty, runtimeVariableTy->getPointerTo()->getPointerTo() }, false),
        "variableInitFromTupleLiteral"
    );
    
    declareFunction(
        llvm::FunctionType::get(runtimeVariableTy->getPointerTo(), { runtimeVariableTy->getPointerTo(), int64Ty }, false),
        "variableGetTupleField"
    );
    declareFunction(
        llvm::FunctionType::get(runtimeVariableTy->getPointerTo(), { runtimeVariableTy->getPointerTo(), int64Ty }, false),
        "variableGetTupleFieldFromID"
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