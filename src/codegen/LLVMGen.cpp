#include "LLVMGen.h"

namespace gazprea
{

    LLVMGen::LLVMGen(
        std::shared_ptr<SymbolTable> symtab,
        std::string &outfile)
        : globalCtx(), ir(globalCtx), mod("gazprea", globalCtx), outfile(outfile),
          llvmFunction(&globalCtx, &ir, &mod),
          llvmBranch(&globalCtx, &ir, &mod),
          numExprAncestors(0)
    {

        runtimeTypeTy = llvm::StructType::create(
            globalCtx,
            {
                ir.getInt32Ty(),
                ir.getInt8PtrTy() // llvm does not have void*, the equivalent is int8*
            },
            "RuntimeType");

        runtimeVariableTy = llvm::StructType::create(
            globalCtx,
            {
                runtimeTypeTy->getPointerTo(),
                ir.getInt8PtrTy(), // llvm does not have void*, the equivalent is int8*
                ir.getInt64Ty(),
                ir.getInt8PtrTy() // llvm does not have void*, the equivalent is int8*
            },
            "RuntimeVariable");
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
            switch (t->getNodeType()) {
            case GazpreaParser::PROCEDURE:
            case GazpreaParser::FUNCTION:
                visitSubroutineDeclDef(t);
                break;
            case GazpreaParser::RETURN:
                visitReturn(t);
                break;
            case GazpreaParser::CONDITIONAL_STATEMENT_TOKEN:
                visitConditionalStatement(t);
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
            case GazpreaParser::GENERATOR_TOKEN:
                visitGenerator(t);
                break;
            case GazpreaParser::FILTER_TOKEN:
                visitFilter(t);
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
            case GazpreaParser::CALL_PROCEDURE_STATEMENT_TOKEN:
                visitCallSubroutineStatement(t);
                break;
            case GazpreaParser::IDENTIFIER_TOKEN:
                visitIdentifier(t);
                break;

            case GazpreaParser::VAR_DECLARATION_TOKEN:
                visitVarDeclarationStatement(t);
                break;
            case GazpreaParser::ASSIGNMENT_TOKEN:
                visitAssignmentStatement(t);
                break;
            case GazpreaParser::UNQUALIFIED_TYPE_TOKEN:
                visitUnqualifiedType(t);
                break;
            case GazpreaParser::PARAMETER_ATOM_TOKEN:
                visitParameterAtom(t);
                break;
            case GazpreaParser::BREAK:
                visitBreak(t);
                break;
            case GazpreaParser::CONTINUE:
                visitContinue(t);
                break;
            default: // The other nodes we don't care about just have their children visited
                visitChildren(t);
            }
        }
    }

    void LLVMGen::visitChildren(std::shared_ptr<AST> t) {
        for (auto child : t->children) visit(child);
    }

    void LLVMGen::visitSubroutineDeclDef(std::shared_ptr<AST> t) {
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->symbol);
        std::vector<llvm::Type *> parameterTypes = std::vector<llvm::Type *>(
            subroutineSymbol->orderedArgs.size(), runtimeVariableTy->getPointerTo());

        llvm::Type *returnType = runtimeVariableTy->getPointerTo();
        if (t->children[2]->isNil()) {
            returnType = ir.getVoidTy();
        }
        else if (subroutineSymbol->name == "main") {
            returnType = ir.getInt32Ty();
        }

        llvm::FunctionType *subroutineTy = llvm::FunctionType::get(
            returnType,
            parameterTypes,
            false);
        auto subroutine = llvm::cast<llvm::Function>(mod.getOrInsertFunction(subroutineSymbol->name, subroutineTy).getCallee());
        subroutineSymbol->llvmFunction = subroutine;

        if (t->children[3]->getNodeType() == GazpreaParser::SUBROUTINE_EMPTY_BODY_TOKEN){
            // subroutine declaration;
        }
        else if (t->children[3]->getNodeType() == GazpreaParser::SUBROUTINE_EXPRESSION_BODY_TOKEN) {
            // Expression Body
            currentSubroutine = subroutine;
            llvm::BasicBlock *bb = llvm::BasicBlock::Create(globalCtx, "enterSubroutine", currentSubroutine);
            ir.SetInsertPoint(bb);

            // Populate Argument Symbol's llvmValue
            visit(t->children[1]);
            for (size_t i = 0; i < subroutineSymbol->orderedArgs.size(); i++) {
                auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(subroutineSymbol->orderedArgs[i]);
                if (variableSymbol->typeQualifier == "const") {
                    auto runtimeVariableParameterObject = llvmFunction.call("variableMalloc", {});
                    llvmFunction.call("variableInitFromParameter", { runtimeVariableParameterObject, variableSymbol->llvmPointerToTypeObject, subroutine->getArg(i) });
                    variableSymbol->llvmPointerToVariableObject = runtimeVariableParameterObject;
                } else {
                    variableSymbol->llvmPointerToVariableObject = subroutine->getArg(i);
                }
                
            }
            visit(t->children[3]);

            if (t->children[2]->isNil()) {
                ir.CreateRetVoid();
            } else {
                if (subroutineSymbol->name == "main") {
                    auto returnIntegerValue = llvmFunction.call("variableGetIntegerValue", {t->children[3]->children[0]->llvmValue});
                    ir.CreateRet(returnIntegerValue);
                } else {
                    ir.CreateRet(t->children[3]->children[0]->llvmValue);
                }
            }
        }
        else {
            // subroutine declaration and definition;
            currentSubroutine = subroutine;
            llvm::BasicBlock *bb = llvm::BasicBlock::Create(globalCtx, "enterSubroutine", currentSubroutine);
            ir.SetInsertPoint(bb);

            // Populate Argument Symbol's llvmValue
            visit(t->children[1]);
            for (size_t i = 0; i < subroutineSymbol->orderedArgs.size(); i++) {
                auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(subroutineSymbol->orderedArgs[i]);
                if (variableSymbol->typeQualifier == "const") {
                    auto runtimeVariableParameterObject = llvmFunction.call("variableMalloc", {});
                    llvmFunction.call("variableInitFromParameter", { runtimeVariableParameterObject, variableSymbol->llvmPointerToTypeObject, subroutine->getArg(i) });
                    variableSymbol->llvmPointerToVariableObject = runtimeVariableParameterObject;
                } else {
                    variableSymbol->llvmPointerToVariableObject = subroutine->getArg(i);
                }
                
            }
            visit(t->children[3]);

            if (t->children[2]->isNil()) {
                ir.CreateRetVoid();
            }
        }
    }

    void LLVMGen::visitReturn(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->scope);
        if (subroutineSymbol->name == "main") {
            auto returnIntegerValue = llvmFunction.call("variableGetIntegerValue", {t->children[0]->llvmValue});
            ir.CreateRet(returnIntegerValue);
            return;
        }
        ir.CreateRet(t->children[0]->llvmValue);
    }

    void LLVMGen::visitVarDeclarationStatement(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(t->symbol);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        if (t->children[2]->isNil()) {
            // No expression => Initialize to null
            auto runtimeTypeObject = t->children[0]->children[1]->llvmValue;
            auto rhs = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromNullScalar", { rhs });
            llvmFunction.call("variableInitFromDeclaration", { runtimeVariableObject, runtimeTypeObject, rhs });
        } else if (t->children[0]->getNodeType() == GazpreaParser::INFERRED_TYPE_TOKEN) {
            auto runtimeTypeObject = llvmFunction.call("typeMalloc", {});
            llvmFunction.call("typeInitFromUnknownType", { runtimeTypeObject });
            llvmFunction.call("variableInitFromDeclaration", {runtimeVariableObject, runtimeTypeObject, t->children[2]->llvmValue});
            variableSymbol->llvmPointerToTypeObject = runtimeTypeObject;
        } else {
            auto runtimeTypeObject = t->children[0]->children[1]->llvmValue;
            llvmFunction.call("variableInitFromDeclaration", {runtimeVariableObject, runtimeTypeObject, t->children[2]->llvmValue});
            variableSymbol->llvmPointerToTypeObject = runtimeTypeObject;
        }
        
        t->llvmValue = runtimeVariableObject;
        variableSymbol->llvmPointerToVariableObject = runtimeVariableObject;
    }

    void LLVMGen::visitAssignmentStatement(std::shared_ptr<AST> t) {
        visitChildren(t);
        llvmFunction.call("variableAssignment", {t->children[0]->children[0]->llvmValue, t->children[1]->llvmValue});
    }

    void LLVMGen::visitUnqualifiedType(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto runtimeTypeObject = llvmFunction.call("typeMalloc", {});

        std::shared_ptr<MatrixType> matrixType;
        llvm::Value *baseType;
        llvm::Value *dimension1Expression = nullptr;
        llvm::Value *dimension2Expression = nullptr;

        switch(t->type->getTypeId()) {
            case Type::BOOLEAN:
                llvmFunction.call("typeInitFromBooleanScalar", {runtimeTypeObject});
                break;
            case Type::CHARACTER:
                llvmFunction.call("typeInitFromCharacterScalar", {runtimeTypeObject});
                break;
            case Type::INTEGER:
                llvmFunction.call("typeInitFromIntegerScalar", {runtimeTypeObject});
                break;
            case Type::REAL:
                llvmFunction.call("typeInitFromRealScalar", {runtimeTypeObject});
                break;
            case Type::INTEGER_INTERVAL:
                llvmFunction.call("typeInitFromIntegerInterval", {runtimeTypeObject});
                break;
            
            case Type::BOOLEAN_1:
                matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromBooleanScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                break;
            
            case Type::CHARACTER_1:
                matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromCharacterScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                break;
            
            case Type::INTEGER_1:
                matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromIntegerScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                break;
            
            case Type::REAL_1:
                matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromRealScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                break;
            
            case Type::BOOLEAN_2:
                matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromBooleanScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", { runtimeTypeObject, dimension1Expression, dimension2Expression, baseType });
                break;
            case Type::CHARACTER_2:
                matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromCharacterScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", { runtimeTypeObject, dimension1Expression, dimension2Expression, baseType });
                break;

            case Type::INTEGER_2:
                matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromIntegerScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", { runtimeTypeObject, dimension1Expression, dimension2Expression, baseType });
                break;
            
            case Type::REAL_2:
                matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromRealScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", { runtimeTypeObject, dimension1Expression, dimension2Expression, baseType });
                break;
        }

        t->llvmValue = runtimeTypeObject;
    }

    void LLVMGen::visitConditionalStatement(std::shared_ptr<AST> t) {
        //Setup 
        auto *ctx = dynamic_cast<GazpreaParser::ConditionalStatementContext*>(t->parseTree);   
        int numChildren = t->children.size();
        //Parent Function 
        llvm::Function* parentFunc = ir.GetInsertBlock()->getParent(); 
        //Initialize If Header and Body
        llvm::BasicBlock* IfHeaderBB = llvm::BasicBlock::Create(globalCtx, "IfHeaderBlock", parentFunc);
        llvm::BasicBlock* IfBodyBB = llvm::BasicBlock::Create(globalCtx, "IfBodyBlock", parentFunc);
        llvm::BasicBlock* ElseIfHeader = llvm::BasicBlock::Create(globalCtx, "ElseIfHeader"); //only use if needed
        llvm::BasicBlock* ElseBlock = llvm::BasicBlock::Create(globalCtx, "ElseBlock"); // only use if needed
        llvm::BasicBlock* Merge = llvm::BasicBlock::Create(globalCtx, "Merge"); //used at end
        // Start inserting at If Header 
        ir.CreateBr(IfHeaderBB);
        ir.SetInsertPoint(IfHeaderBB);
        visit(t->children[0]); 
        // setup branch condition
        auto exprValue = llvmFunction.call("variableGetBooleanValue", {t->children[0]->llvmValue}); //HERE
        llvm::Value* ifCondition = ir.CreateICmpNE(exprValue, ir.getInt32(0));
        if ((!ctx->elseStatement() && numChildren > 2) || (ctx->elseStatement() && numChildren > 3)) {
            ir.CreateCondBr(ifCondition, IfBodyBB, ElseIfHeader); 
        } else if(ctx->elseStatement()) {
            ir.CreateCondBr(ifCondition, IfBodyBB, ElseBlock); 
        } else {
            ir.CreateCondBr(ifCondition, IfBodyBB, Merge); 
        }
        //If Body
        ir.SetInsertPoint(IfBodyBB);
        visit(t->children[1]);
        ir.CreateBr(Merge);
        // Handle Arbitrary Number of Else If
        int elseIfIdx = 2; // 0 is if expr  1 is if body 2 starts else if
        llvm::BasicBlock* residualElseIfHeader = nullptr; //we declare next else if header in previous itteration because we need it for the conditional branch
        for (auto elseIfStatement : ctx->elseIfStatement()) { 
            auto elifNode = t->children[elseIfIdx];            
            if(residualElseIfHeader != nullptr) { //first else if 
                ir.SetInsertPoint(residualElseIfHeader);
            }else {
                parentFunc->getBasicBlockList().push_back(ElseIfHeader);
                ir.SetInsertPoint(ElseIfHeader);
            }
            //Fill header
            visit(elifNode->children[0]); 
            auto elseIfExprValue = llvmFunction.call("variableGetBooleanValue", {elifNode->children[0]->llvmValue});
            llvm::Value* elseIfCondition = ir.CreateICmpNE(elseIfExprValue, ir.getInt32(0));
            llvm::BasicBlock* elseIfBodyBlock = llvm::BasicBlock::Create(globalCtx, "ElseIfBody", parentFunc);
            // Conditoinal Branch Out (3 Cases)
            if (!ctx->elseStatement() && elseIfIdx == (numChildren -1)) {           // 1) last else if no else
                ir.CreateCondBr(elseIfCondition, elseIfBodyBlock, Merge); 
            } else if (ctx->elseStatement() && elseIfIdx == (numChildren -2)) {     // 2) last else if w/ else
                ir.CreateCondBr(elseIfCondition, elseIfBodyBlock, ElseBlock);  
            } else {                                                                // 3) another else if
                llvm::BasicBlock* nextElseIfHeaderBlock = llvm::BasicBlock::Create(globalCtx, "nextElseIfHeader", parentFunc);
                ir.CreateCondBr(elseIfCondition, elseIfBodyBlock, nextElseIfHeaderBlock); 
                residualElseIfHeader = nextElseIfHeaderBlock;
            } 
            //Fill Body
            ir.SetInsertPoint(elseIfBodyBlock);
            visit(elifNode->children[1]);
            ir.CreateBr(Merge);
            elseIfIdx++; //Elif Counter
        } 
        // Else Caluse (Optional) 
        if (ctx->elseStatement()) {
            parentFunc->getBasicBlockList().push_back(ElseBlock);
            int elseIdx = t->children.size()-1;
            auto elseNode = t->children[elseIdx]->children[0]; 
            ir.SetInsertPoint(ElseBlock);
            visit(elseNode);
            ir.CreateBr(Merge);
        }
        // Merge
        parentFunc->getBasicBlockList().push_back(Merge); 
        ir.SetInsertPoint(Merge);         
    }
    
    void LLVMGen::visitBreak(std::shared_ptr<AST> t) {
        int stackSize = llvmBranch.blockStack.size();
        llvm::Function *parentFunc = ir.GetInsertBlock()->getParent();
        llvm::BasicBlock* mergeBB = llvmBranch.blockStack[stackSize -1]; 
        llvm::BasicBlock* breakBlock = llvm::BasicBlock::Create(globalCtx, "BreakBlock", parentFunc);
        llvm::BasicBlock* resumeLoopBody = llvm::BasicBlock::Create(globalCtx, "ResumeLoop", parentFunc);

        ir.CreateBr(breakBlock);
        ir.SetInsertPoint(breakBlock);
        ir.CreateBr(mergeBB);
        ir.SetInsertPoint(resumeLoopBody);
    }

    void LLVMGen::visitContinue(std::shared_ptr<AST> t) {
        int stackSize = llvmBranch.blockStack.size();
        llvm::Function *parentFunc = ir.GetInsertBlock()->getParent();
        llvm::BasicBlock* loopHeader = llvmBranch.blockStack[stackSize -3]; 
        llvm::BasicBlock* continueBlock = llvm::BasicBlock::Create(globalCtx, "ContinueBlock", parentFunc);
        llvm::BasicBlock* resumeLoopBody = llvm::BasicBlock::Create(globalCtx, "ResumeLoop", parentFunc);

        ir.CreateBr(continueBlock);
        ir.SetInsertPoint(continueBlock);
        ir.CreateBr(loopHeader);
        ir.SetInsertPoint(resumeLoopBody);
    }
    
    void LLVMGen::viistInfiniteLoop(std::shared_ptr<AST> t) {
        // setup
        llvm::Function *parentFunc = ir.GetInsertBlock()->getParent();
        llvm::BasicBlock *InfiniteBodyBB = llvm::BasicBlock::Create(globalCtx, "InfiniteBody", parentFunc);
        llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(globalCtx, "MergeFromInfinteLoop");
        llvmBranch.blockStack.push_back(InfiniteBodyBB);
        llvmBranch.blockStack.push_back(nullptr); // maintain index offset to other loop methods
        llvmBranch.blockStack.push_back(MergeBB);
        // create infinite loop
        ir.CreateBr(InfiniteBodyBB);
        ir.SetInsertPoint(InfiniteBodyBB);
        visitChildren(t);
        parentFunc->getBasicBlockList().push_back(MergeBB);
        ir.CreateBr(InfiniteBodyBB);
        ir.SetInsertPoint(MergeBB);
        // keep stack organized
        llvmBranch.blockStack.pop_back();
        llvmBranch.blockStack.pop_back();
        llvmBranch.blockStack.pop_back();
    }

    void LLVMGen::visitPrePredicatedLoop(std::shared_ptr<AST> t) {
        auto runtimeVarConstZero = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromIntegerScalar", {runtimeVarConstZero, ir.getInt32(0)}); //Type must match for ICmpNE
        llvmBranch.createPrePredConditionalBB("PrePredLoop");
        visit(t->children[0]);      // Conditional Expr
        auto exprValue = llvmFunction.call("variableGetBooleanValue", {t->children[0]->llvmValue});
        llvm::Value* condition = ir.CreateICmpNE(exprValue, ir.getInt32(0)); 
        llvmBranch.createPrePredBodyBB(condition);
        visit(t->children[1]);      // Visit body
        llvmBranch.createPrePredMergeBB();
    }

    void LLVMGen::visitPostPredicatedLoop(std::shared_ptr<AST> t) {
        auto runtimeVarConstZero = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromIntegerScalar", {runtimeVarConstZero, ir.getInt32(0)}); //Type must match for ICmpNE 
        llvmBranch.createPostPredBodyBB(); 
        visit(t->children[0]);      //visit Body 
        llvmBranch.createPostPredConditionalBB(); 
        visit(t->children[1]);      //grab value from post predicate
        auto exprValue = llvmFunction.call("variableGetBooleanValue", {t->children[1]->llvmValue});
        llvm::Value *condition = ir.CreateICmpNE(exprValue, ir.getInt32(0));
        llvmBranch.createPostPredMergeBB(condition);
    }

    void LLVMGen::visitIteratorLoop(std::shared_ptr<AST> t) {
        visitChildren(t);
        // TODO
    }

    void LLVMGen::visitBooleanAtom(std::shared_ptr<AST> t) {
        bool booleanValue = 0;
        if (t->parseTree->getText() == "true") {
            booleanValue = 1;
        }
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromBooleanScalar", {runtimeVariableObject, ir.getInt32(booleanValue)});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitCharacterAtom(std::shared_ptr<AST> t) {
        auto characterValue = t->parseTree->getText()[1];
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromCharacterScalar", {runtimeVariableObject, ir.getInt8(characterValue)});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitIntegerAtom(std::shared_ptr<AST> t) {
        auto integerValue = std::stoi(t->parseTree->getText());
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromIntegerScalar", {runtimeVariableObject, ir.getInt32(integerValue)});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitRealAtom(std::shared_ptr<AST> t) {
        auto realValue = std::stof(t->parseTree->getText());
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromRealScalar", {runtimeVariableObject, llvm::ConstantFP::get(ir.getFloatTy(), realValue)});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitIdentityAtom(std::shared_ptr<AST> t) {
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromIdentityScalar", {runtimeVariableObject});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitNullAtom(std::shared_ptr<AST> t) {
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromNullScalar", {runtimeVariableObject});
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
        if (numExprAncestors > 0)
        {
            t->llvmValue = t->symbol->llvmPointerToVariableObject;
        }
    }

    void LLVMGen::visitGenerator(std::shared_ptr<AST> t) {
        visitChildren(t);
        // TODO
    }

    void LLVMGen::visitFilter(std::shared_ptr<AST> t) {
        visitChildren(t);
        // TODO
    }

    void LLVMGen::visitExpression(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->llvmValue = t->children[0]->llvmValue;
    }

    void LLVMGen::visitCast(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromCast", { runtimeVariableObject, t->children[0]->llvmValue, t->children[1]->llvmValue });
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitInputStreamStatement(std::shared_ptr<AST> t) {
        visitChildren(t);
        // TODO
    }

    void LLVMGen::visitOutputStreamStatement(std::shared_ptr<AST> t)
    {
        visitChildren(t);
        llvmFunction.call("variablePrintToStdout", {t->children[0]->llvmValue});
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
        llvmFunction.call("variableInitFromBinaryOp", {runtimeVariableObject, t->children[0]->llvmValue, t->children[1]->llvmValue, ir.getInt32(opCode)});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitUnaryOperation(std::shared_ptr<AST> t) {
        visitChildren(t);
        int opCode;
        switch (t->children[0]->getNodeType()) {
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
        llvmFunction.call("variableInitFromUnaryOp", {runtimeVariableObject, t->children[1]->llvmValue, ir.getInt32(opCode)});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitIndexing(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromBinaryOp", {runtimeVariableObject, t->children[0]->llvmValue, t->children[1]->llvmValue, ir.getInt32(0)});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitInterval(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromBinaryOp", {runtimeVariableObject, t->children[0]->llvmValue, t->children[1]->llvmValue, ir.getInt32(1)});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitStringConcatenation(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromBinaryOp", {runtimeVariableObject, t->children[0]->llvmValue, t->children[1]->llvmValue, ir.getInt32(19)});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitCallSubroutineInExpression(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->children[0]->symbol);
        std::vector<llvm::Value *> arguments = std::vector<llvm::Value *>();
        if (!t->children[1]->isNil())
        {
            for (auto expressionAST : t->children[1]->children)
            {
                arguments.push_back(expressionAST->llvmValue);
            }
        }
        auto llvmReturnValue = ir.CreateCall(subroutineSymbol->llvmFunction, arguments);
        t->llvmValue = llvmReturnValue;
    }

    void LLVMGen::visitCallSubroutineStatement(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->children[0]->symbol);
        std::vector<llvm::Value *> arguments = std::vector<llvm::Value *>();
        if (!t->children[1]->isNil())
        {
            for (auto expressionAST : t->children[1]->children)
            {
                arguments.push_back(expressionAST->llvmValue);
            }
        }
        ir.CreateCall(subroutineSymbol->llvmFunction, arguments);
    }

    void LLVMGen::visitParameterAtom(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(t->symbol);

        auto runtimeTypeObject = llvmFunction.call("typeMalloc", {});

        // std::shared_ptr<MatrixType> matrixType;
        // llvm::Value *baseType;
        // llvm::Value *dimension1Expression = nullptr;
        // llvm::Value *dimension2Expression = nullptr;

        switch(variableSymbol->type->getTypeId()) {
            case Type::BOOLEAN:
                llvmFunction.call("typeInitFromBooleanScalar", { runtimeTypeObject });
                break;
            case Type::CHARACTER:
                llvmFunction.call("typeInitFromCharacterScalar", { runtimeTypeObject });
                break;
            case Type::INTEGER:
                llvmFunction.call("typeInitFromIntegerScalar", { runtimeTypeObject });
                break;
            case Type::REAL:
                llvmFunction.call("typeInitFromRealScalar", { runtimeTypeObject });
                break;
            case Type::INTEGER_INTERVAL:
                llvmFunction.call("typeInitFromIntegerInterval", { runtimeTypeObject });
                break;
            // TODO: Other Types
        }
        variableSymbol->llvmPointerToTypeObject = runtimeTypeObject;
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