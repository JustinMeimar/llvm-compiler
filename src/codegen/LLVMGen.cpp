#include "LLVMGen.h"

namespace gazprea {

LLVMGen::LLVMGen(std::shared_ptr<SymbolTable> symtab, std::string& outfile) : 
    globalCtx(), ir(globalCtx), mod("gazprea", globalCtx), outfile(outfile), types(&globalCtx) {}

LLVMGen::~LLVMGen() { 
    Print();
}

void LLVMGen::visit(std::shared_ptr<AST> t) { 
    if (t->isNil()) {
        visitChildren(t);
    } else {
        switch(t->getNodeType()) {
            case GazpreaParser::PROCEDURE:
            case GazpreaParser::FUNCTION:
                visitSubroutineDeclDef(t);
                break;
            case GazpreaParser::RETURN:
                visitReturn(t);
            default: // The other nodes we don't care about just have their children visited
                visitChildren(t);
        }
    }
}

void LLVMGen::visitChildren(std::shared_ptr<AST> t) {
    for( auto child : t->children ) visit(child);
}

void LLVMGen::visitSubroutineDeclDef(std::shared_ptr<AST> t) {
    
    auto subroutineName = t->symbol->getName(); 
    auto subroutineReturnTy = t->symbol->type->getTypeId(); 

    llvm::Type* tempTy = types.getType(Type::INTEGER); //get llvm type from Type Enum 
    llvm::Function* subroutine;
    subroutine = llvm::cast<llvm::Function>(mod.getOrInsertFunction(subroutineName, tempTy).getCallee());  
    
    if(t->children[4] == nullptr) {
        //subroutine declaration;
        return;

    } else {
        //subroutine declaration and definition;
        llvm::BasicBlock* bb = llvm::BasicBlock::Create(globalCtx, "enterSubroutine", subroutine); 
        ir.SetInsertPoint(bb);
        visitChildren(t);
        ir.CreateRet(ir.getInt32(0)); //probably will be responsibility of visit chidlren
        return;
    }
}

void LLVMGen::visitReturn(std::shared_ptr<AST> t) {
    visitChildren(t);
    auto returnTy = t->children[0]->evalType->getTypeId();
    std::cout << returnTy << std::endl;
}

void LLVMGen::Print() {
    // write module as .ll file
    std::string compiledLLFile;
    llvm::raw_string_ostream out(compiledLLFile);
    
    llvm::raw_os_ostream llOut(std::cout);
    llvm::raw_os_ostream llErr(std::cerr);
    llvm::verifyModule(mod, &llErr);
 
    mod.print(out, nullptr);
    out.flush();
    std::ofstream outFile(outfile);
    outFile << compiledLLFile;
}



} // namespace gazprea