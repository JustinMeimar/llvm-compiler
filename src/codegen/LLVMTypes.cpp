#include <iostream>
#include <map>
#include <Type.h>
#include "LLVMGen.h"

namespace gazprea {

LLVMTypes::LLVMTypes(llvm::LLVMContext *globalCtx) : context(globalCtx) {}

llvm::Type* LLVMTypes::getType(size_t enumTy){
    switch(enumTy){
        case Type::TUPLE:       return nullptr; break;
        case Type::CHARACTER:   return llvm::Type::getInt8Ty(*context);     break;
        case Type::INTEGER:     return llvm::Type::getInt32Ty(*context);    break;
        case Type::REAL:        return llvm::Type::getFloatTy(*context);    break;
    }
    return nullptr;
}

}