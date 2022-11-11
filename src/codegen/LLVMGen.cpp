#include "LLVMGen.h"

namespace gazprea {

    LLVMGen::LLVMGen(
        std::shared_ptr<SymbolTable> symtab, 
        std::string& outfile) 
        : globalCtx(), ir(globalCtx), mod("gazprea", globalCtx), outfile(outfile), 
        llvmFunction(&globalCtx, &ir, &mod), 
        llvmBranch(&globalCtx, &ir, &mod),
        numExprAncestors(0) {
        
        runtimeTypeTy = llvm::StructType::create(
            globalCtx,
            {
                ir.getInt32Ty(),
                ir.getInt8PtrTy()  // llvm does not have void*, the equivalent is int8*
            },
            "RuntimeType"
        );
        
        runtimeVariableTy = llvm::StructType::create(
            globalCtx, 
            {
                runtimeTypeTy->getPointerTo(), 
                ir.getInt8PtrTy(),  // llvm does not have void*, the equivalent is int8*
                ir.getInt64Ty(),
                ir.getInt8PtrTy()  // llvm does not have void*, the equivalent is int8*
            }, 
            "RuntimeVariable"
        );
        llvmFunction.runtimeTypeTy = runtimeTypeTy;
        llvmFunction.runtimeVariableTy = runtimeVariableTy;
        llvmFunction.declareAllFunctions();
    }

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
                    break;
                case GazpreaParser::INFINITE_LOOP_TOKEN:
                    viistInfiniteLoop(t);
                    break;
                case GazpreaParser::PRE_PREDICATE_LOOP_TOKEN:
                    visitPrePredicatedLoop(t);
                    break;
                case GazpreaParser::POST_PREDICATE_LOOP_TOKEN:
                    visitPostPredicatedLoop(t); 
                    break;
                case GazpreaParser::ITERATOR_LOOP_TOKEN:
                    visitIteratorLoop(t);
                    break;  
                case GazpreaParser::BooleanConstant:
                    visitBooleanAtom(t);
                    break;
                case GazpreaParser::CharacterConstant:
                    visitCharacterAtom(t);
                    break;
                case GazpreaParser::IntegerConstant:
                    visitIntegerAtom(t);
                    break;
                case GazpreaParser::REAL_CONSTANT_TOKEN:
                    visitRealAtom(t);
                    break;
                case GazpreaParser::IDENTITY:
                    visitIdentityAtom(t);
                    break;
                case GazpreaParser::NULL_LITERAL:
                    visitNullAtom(t);
                    break;
                case GazpreaParser::StringLiteral:
                    visitStringLiteral(t);
                    break;
                case GazpreaParser::EXPRESSION_TOKEN:
                    numExprAncestors++;
                    visitExpression(t);
                    numExprAncestors--;
                    break;
                case GazpreaParser::CAST_TOKEN:
                    visitCast(t);
                    break;
                case GazpreaParser::INPUT_STREAM_TOKEN:
                    visitInputStreamStatement(t);
                    break;
                case GazpreaParser::OUTPUT_STREAM_TOKEN:
                    visitOutputStreamStatement(t);
                    break;
                case GazpreaParser::UNARY_TOKEN:
                    visitUnaryOperation(t);
                    break;
                case GazpreaParser::BINARY_OP_TOKEN:
                    visitBinaryOperation(t);
                    break;
                case GazpreaParser::INDEXING_TOKEN:
                    visitIndexing(t);
                    break;
                case GazpreaParser::INTERVAL:
                    visitInterval(t);
                    break;
                case GazpreaParser::STRING_CONCAT_TOKEN:
                    visitStringConcatenation(t);
                    break;
                case GazpreaParser::CALL_PROCEDURE_FUNCTION_IN_EXPRESSION:
                    visitCallSubroutineInExpression(t);
                    break;
                case GazpreaParser::IDENTIFIER_TOKEN:
                    visitIdentifier(t);
                    break;
                default: // The other nodes we don't care about just have their children visited
                    visitChildren(t);
            }
        }
    }

    void LLVMGen::visitChildren(std::shared_ptr<AST> t) {
        for( auto child : t->children ) visit(child);
    }

    void LLVMGen::visitSubroutineDeclDef(std::shared_ptr<AST> t) {
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->symbol);
        std::vector<llvm::Type *> parameterTypes = std::vector<llvm::Type *>(
            subroutineSymbol->orderedArgs.size(), runtimeVariableTy->getPointerTo()
        );

        llvm::Type *returnType = runtimeVariableTy->getPointerTo();
        if (t->children[2]->isNil()) {
            returnType = ir.getVoidTy();
        } else if (subroutineSymbol->name == "main") {
            returnType = ir.getInt32Ty();
        }

        llvm::FunctionType* subroutineTy = llvm::FunctionType::get(
            returnType,
            parameterTypes,
            false
        );
        auto subroutine = llvm::cast<llvm::Function>(mod.getOrInsertFunction(subroutineSymbol->name, subroutineTy).getCallee());
        subroutineSymbol->llvmFunction = subroutine; 
        
        if(t->children[3]->getNodeType() == GazpreaParser::SUBROUTINE_EMPTY_BODY_TOKEN) {
            //subroutine declaration;
        } else if (t->children[3]->getNodeType() == GazpreaParser::SUBROUTINE_EXPRESSION_BODY_TOKEN) {
            // Expression Body
            currentSubroutine = subroutine;
            llvm::BasicBlock* bb = llvm::BasicBlock::Create(globalCtx, "enterSubroutine", currentSubroutine);
            ir.SetInsertPoint(bb);
            visitChildren(t);
            
            if (t->children[2]->isNil()) {
                ir.CreateRetVoid();
            } else {
                if (subroutineSymbol->name == "main") {
                    auto returnIntegerValue = llvmFunction.call("variableGetIntegerValue", { t->children[3]->children[0]->llvmValue });
                    ir.CreateRet(returnIntegerValue);
                } else {
                    ir.CreateRet(ir.CreateLoad(runtimeVariableTy, t->children[3]->children[0]->llvmValue));
                }
            }
        } else {
            //subroutine declaration and definition;
            currentSubroutine = subroutine;
            llvm::BasicBlock* bb = llvm::BasicBlock::Create(globalCtx, "enterSubroutine", currentSubroutine); 
            ir.SetInsertPoint(bb);
            visitChildren(t);
            
            if (t->children[2]->isNil()) {
                ir.CreateRetVoid();
            }
        }
    }

    void LLVMGen::visitReturn(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->scope);
        if (subroutineSymbol->name == "main") {
            auto returnIntegerValue = llvmFunction.call("variableGetIntegerValue", { t->children[0]->llvmValue });
            ir.CreateRet(returnIntegerValue);
            return;
        }
        // ir.CreateRet(ir.CreateLoad(runtimeVariableTy, t->children[0]->llvmValue));
        ir.CreateRet(t->children[0]->llvmValue);
    }

    void LLVMGen::viistInfiniteLoop(std::shared_ptr<AST> t) {
        std::cout << "Infinite Loop!\n";

        llvm::Function* parentFunc = ir.GetInsertBlock()->getParent(); 
        llvm::BasicBlock *EnterBodyBB = llvm::BasicBlock::Create(
                globalCtx, "InfiniteBody", parentFunc);

        ir.CreateBr(EnterBodyBB);
        ir.SetInsertPoint(EnterBodyBB);
        visitChildren(t);

        ir.CreateBr(EnterBodyBB); 
    }
    
    void LLVMGen::visitPrePredicatedLoop(std::shared_ptr<AST> t) {
        std::cout << "Pre Predicated Loop!\n";

        llvmBranch.createLoopConditionalBB("Loop");
        visit(t->children[0]);
        llvm::Value* exprValue = ir.getInt32(1); //llvmValue of AST not populated, temporary true;
        // llvm::Value* exprValue = t->children[0]->llvmValue; 
        llvm::Value *condition = ir.CreateICmpNE(exprValue, ir.getInt32(0));

        llvmBranch.createLoopBodyBB(condition);
        visit(t->children[1]);  // Visit body
        
	    llvmBranch.createLoopMergeBB();
    }
    
    void LLVMGen::visitPostPredicatedLoop(std::shared_ptr<AST> t) {
        
    }
    
    void LLVMGen::visitIteratorLoop(std::shared_ptr<AST> t) {

    } 

    void LLVMGen::visitBooleanAtom(std::shared_ptr<AST> t) {
        bool booleanValue;
        if (t->parseTree->getText() == "true") {
            booleanValue = 1;
        } else {
            booleanValue = 0;
        }
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromBooleanScalar", { runtimeVariableObject, ir.getInt1(booleanValue) });
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitCharacterAtom(std::shared_ptr<AST> t) {
        auto characterValue = t->parseTree->getText()[1];
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromCharacterScalar", { runtimeVariableObject, ir.getInt8(characterValue) });
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitIntegerAtom(std::shared_ptr<AST> t) {
        auto integerValue = std::stoi(t->parseTree->getText());
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromIntegerScalar", { runtimeVariableObject, ir.getInt32(integerValue) });
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitRealAtom(std::shared_ptr<AST> t) {
        auto realValue = std::stof(t->parseTree->getText());
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromRealScalar", { runtimeVariableObject, llvm::ConstantFP::get(ir.getFloatTy(), realValue) });
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitIdentityAtom(std::shared_ptr<AST> t) {
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromIdentityScalar", { runtimeVariableObject });
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitNullAtom(std::shared_ptr<AST> t) {
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromNullScalar", { runtimeVariableObject });
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitStringLiteral(std::shared_ptr<AST> t) {
        // TODO
        // auto string_value = t->parseTree->getText().substr(1, t->parseTree->getText().length() - 2).c_str();
        // auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        // auto string_length = t->parseTree->getText().length();
        
        // llvmFunction.call("variableInitFromString", { runtimeVariableObject, ir.getInt64(string_length), string_value } );
        // t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitIdentifier(std::shared_ptr<AST> t) {
        visitChildren(t);
        if (numExprAncestors > 0) {
            t->llvmValue = t->symbol->llvmPointerToVariableObject;
        }   
    }

    void LLVMGen::visitExpression(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->llvmValue = t->children[0]->llvmValue;
    }
    
    void LLVMGen::visitCast(std::shared_ptr<AST> t) {
        visitChildren(t);
    }

    void LLVMGen::visitInputStreamStatement(std::shared_ptr<AST> t) {
        visitChildren(t);
        // TODO: Runtime function not implemented
    }

    void LLVMGen::visitOutputStreamStatement(std::shared_ptr<AST> t) {
        visitChildren(t);
        llvmFunction.call("variablePrintToStdout", { t->children[0]->llvmValue });
    }

    void LLVMGen::visitBinaryOperation(std::shared_ptr<AST> t) {
        visitChildren(t);
        int opCode;
        switch (t->children[2]->getNodeType()) {
            // case GazpreaParser::INDEXING_TOKEN:
            //     opCode = 0;
            //     break;
            // case GazpreaParser::INTERVAL:
            //     opCode = 1;
            //     break;
            case GazpreaParser::CARET: // Character '*'
                opCode = 2;
                break;
            case GazpreaParser::ASTERISK:
                opCode = 3;
                break;
            case GazpreaParser::DIV:
                opCode = 4;
                break;
            case GazpreaParser::MODULO:
                opCode = 5;
                break;
            case GazpreaParser::DOTPRODUCT:
                opCode = 6;
                break;
            case GazpreaParser::PLUS:
                opCode = 7;
                break;
            case GazpreaParser::MINUS:
                opCode = 8;
                break;
            case GazpreaParser::BY:
                opCode = 9;
                break;
            case GazpreaParser::LESSTHAN:
                opCode = 10;
                break;
            case GazpreaParser::GREATERTHAN:
                opCode = 11;
                break;
            case GazpreaParser::LESSTHANOREQUAL:
                opCode = 12;
                break;
            case GazpreaParser::GREATERTHANOREQUAL:
                opCode = 13;
                break;
            case GazpreaParser::ISEQUAL:
                opCode = 14;
                break;
            case GazpreaParser::ISNOTEQUAL:
                opCode = 15;
                break;
            case GazpreaParser::AND:
                opCode = 16;
                break;
            case GazpreaParser::OR:
                opCode = 17;
                break;
            case GazpreaParser::XOR:
                opCode = 18;
                break;
            // case GazpreaParser::STRING_CONCAT_TOKEN:
            //     opCode = 19;
            //     break;
            default:
                opCode = -1;
        }
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromBinaryOp", { runtimeVariableObject, t->children[0]->llvmValue, t->children[1]->llvmValue, ir.getInt32(opCode) });
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitUnaryOperation(std::shared_ptr<AST> t) {
        visitChildren(t);
        int opCode;
        switch(t->children[0]->getNodeType()) {
            case GazpreaParser::PLUS:
                opCode = 0;
                break;
            case GazpreaParser::MINUS:
                opCode = 1;
                break;
            default:
                // "not" operator
                opCode = 2;
                break;
        }
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromUnaryOp", { runtimeVariableObject, t->children[1]->llvmValue, ir.getInt32(opCode) });
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitIndexing(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromBinaryOp", { runtimeVariableObject, t->children[0]->llvmValue, t->children[1]->llvmValue, ir.getInt32(0) });
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitInterval(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromBinaryOp", { runtimeVariableObject, t->children[0]->llvmValue, t->children[1]->llvmValue, ir.getInt32(1) });
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitStringConcatenation(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromBinaryOp", { runtimeVariableObject, t->children[0]->llvmValue, t->children[1]->llvmValue, ir.getInt32(19) });
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitCallSubroutineInExpression(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->children[0]->symbol);
        std::vector<llvm::Value *> arguments = std::vector<llvm::Value *>();
        if (!t->children[1]->isNil()) {
            for (auto expressionAST : t->children[1]->children) {
                arguments.push_back(expressionAST->llvmValue);
            }
        }
        auto llvmReturnValue = ir.CreateCall(subroutineSymbol->llvmFunction, arguments);
        t->llvmValue = llvmReturnValue;
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