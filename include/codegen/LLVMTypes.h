#pragma once

#include <iostream>
#include <map>
#include <Type.h>
#include "LLVMGen.h"

namespace gazprea {
    class LLVMTypes {
        public: 

            LLVMTypes(llvm::LLVMContext *globalCtx);
            llvm::LLVMContext *context;

            //Methods
            llvm::Type* getType(size_t enumTy); 
    };
}