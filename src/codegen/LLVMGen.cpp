#include "LLVMGen.h"

namespace gazprea
{

    LLVMGen::LLVMGen(
        std::shared_ptr<SymbolTable> symtab,
        std::string &outfile)
        : symtab(symtab), globalCtx(), ir(globalCtx), mod("gazprea", globalCtx), outfile(outfile),
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

        // Declare Global Variables
        for (auto variableSymbol : symtab->globals->globalVariableSymbols) {
            mod.getOrInsertGlobal(variableSymbol->name, runtimeVariableTy->getPointerTo() );
            auto globalVar = mod.getNamedGlobal(variableSymbol->name);
            globalVar->setLinkage(llvm::GlobalValue::InternalLinkage);
            globalVar->setInitializer(llvm::ConstantPointerNull::get(runtimeVariableTy->getPointerTo()));
            variableSymbol->llvmPointerToVariableObject = globalVar;
        }
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
            case GazpreaParser::TYPEDEF:
                visitTypedef(t);
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
            case GazpreaParser::TUPLE_LITERAL_TOKEN:
                visitTupleLiteral(t);
                break;
            case GazpreaParser::TUPLE_ACCESS_TOKEN: 
                visitTupleAccess(t);
                break;
            case GazpreaParser::VECTOR_LITERAL_TOKEN:
                visitVectorMatrixLiteral(t);
                break;
            case GazpreaParser::BLOCK_TOKEN:
                visitBlock(t);
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
        std::string prefix = subroutineSymbol->name == "main" ? "" : "gazprea.subroutine.";

        auto subroutine = llvm::cast<llvm::Function>(mod.getOrInsertFunction(prefix + subroutineSymbol->name, subroutineTy).getCallee());
        subroutineSymbol->llvmFunction = subroutine;

        if (t->children[3]->getNodeType() == GazpreaParser::SUBROUTINE_EMPTY_BODY_TOKEN) {
            return;
        }

        currentSubroutine = subroutine;
        llvm::BasicBlock *bb = llvm::BasicBlock::Create(globalCtx, "enterSubroutine", currentSubroutine);
        ir.SetInsertPoint(bb);
        if (subroutineSymbol->name == "main") {
            initializeGlobalVariables();
        }
        visit(t->children[1]);  // Populate Argument Symbol's llvmValue
        initializeSubroutineParameters(subroutineSymbol);
        visit(t->children[3]);  // Visit body
        
        if (t->children[3]->getNodeType() == GazpreaParser::SUBROUTINE_EXPRESSION_BODY_TOKEN) {
            // Subroutine with Expression Body
            if (t->children[2]->isNil()) {
                freeSubroutineParameters(subroutineSymbol);
                ir.CreateRetVoid();
                return;
            }

            auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromMemcpy", { runtimeVariableObject, t->children[3]->children[0]->llvmValue });
            
            freeExpressionIfNecessary(t->children[3]->children[0]);
            freeSubroutineParameters(subroutineSymbol);
            
            if (subroutineSymbol->name == "main") {
                auto returnIntegerValue = llvmFunction.call("variableGetIntegerValue", runtimeVariableObject);
                llvmFunction.call("variableDestructThenFree", runtimeVariableObject);
                freeGlobalVariables();
                ir.CreateRet(returnIntegerValue);
            } else {
                ir.CreateRet(runtimeVariableObject);
            }
        } else {
            // subroutine declaration and definition;
            if (t->children[2]->isNil()) {
                // Free all variables in a block if no return statement in a subroutine
                freeSubroutineParameters(subroutineSymbol);
                ir.CreateRetVoid();
            }
        }
    }

    void LLVMGen::visitReturn(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->subroutineSymbol);
        //throw incompatible return type exception
        if(t->children[0]->evalType != nullptr && subroutineSymbol->type->getTypeId() != t->children[0]->evalType->getTypeId()) {
            std::cout << "Subroutine does not return ";
            auto *ctx = dynamic_cast<GazpreaParser::ReturnStatementContext*>(t->parseTree); 
            throw BadReturnTypeError(subroutineSymbol->type->getName(),ctx->getText(), ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine()
            );
        }
        
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromMemcpy", { runtimeVariableObject, t->children[0]->llvmValue });
        
        freeExpressionIfNecessary(t->children[0]);
        freeSubroutineParameters(subroutineSymbol);
        llvmBranch.hitReturnStat = true;
        
        if (subroutineSymbol->name == "main") {
            auto returnIntegerValue = llvmFunction.call("variableGetIntegerValue", { runtimeVariableObject });

            std::shared_ptr<Scope> temp = t->scope;
            while (temp->getEnclosingScope()->getScopeName() != "gazprea.scope.global") {
                freeAllVariablesDeclaredInBlockScope(std::dynamic_pointer_cast<LocalScope>(temp));
                temp = temp->getEnclosingScope();
            }

            freeGlobalVariables();
            llvmFunction.call("variableDestructThenFree", runtimeVariableObject);
            ir.CreateRet(returnIntegerValue);
        } else {
            // Non-main subroutine

            // Recursively free the current scope of return Statement and all of its enclosing scope (traverse up until subroutineSymbol)
            std::shared_ptr<Scope> temp = t->scope;
            while (temp->getEnclosingScope()->getScopeName() != "gazprea.scope.global") {
                // The enclosing scope of the subroutine symbol is global variable
                freeAllVariablesDeclaredInBlockScope(std::dynamic_pointer_cast<LocalScope>(temp));
                temp = temp->getEnclosingScope();
            }
            ir.CreateRet(runtimeVariableObject);
        }
    }

    void LLVMGen::visitVarDeclarationStatement(std::shared_ptr<AST> t) {
        auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(t->symbol);
        if (variableSymbol->isGlobalVariable) {
            return;
        }
        visitChildren(t);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        if (t->children[2]->isNil()) {
            // No expression => Initialize to null
            auto runtimeTypeObject = t->children[0]->children[1]->llvmValue;
            auto rhs = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromNullScalar", { rhs });
            llvmFunction.call("variableInitFromDeclaration", { runtimeVariableObject, runtimeTypeObject, rhs });
            llvmFunction.call("typeDestructThenFree", runtimeTypeObject);
            llvmFunction.call("variableDestructThenFree", rhs);
        } else if (t->children[0]->getNodeType() == GazpreaParser::INFERRED_TYPE_TOKEN) {
            auto runtimeTypeObject = llvmFunction.call("typeMalloc", {});
            llvmFunction.call("typeInitFromUnknownType", { runtimeTypeObject });
            llvmFunction.call("variableInitFromDeclaration", {runtimeVariableObject, runtimeTypeObject, t->children[2]->llvmValue});
            variableSymbol->llvmPointerToTypeObject = runtimeTypeObject;
            
            freeExpressionIfNecessary(t->children[2]);
            llvmFunction.call("typeDestructThenFree", runtimeTypeObject);
        } else {
            auto runtimeTypeObject = t->children[0]->children[1]->llvmValue;
            llvmFunction.call("variableInitFromDeclaration", {runtimeVariableObject, runtimeTypeObject, t->children[2]->llvmValue});
            variableSymbol->llvmPointerToTypeObject = runtimeTypeObject;

            freeExpressionIfNecessary(t->children[2]);
            llvmFunction.call("typeDestructThenFree", runtimeTypeObject);
        }
        
        t->llvmValue = runtimeVariableObject;
        variableSymbol->llvmPointerToVariableObject = runtimeVariableObject;
    }

    void LLVMGen::visitAssignmentStatement(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto numLHSExpressions = t->children[0]->children.size();
        if (numLHSExpressions == 1) {
            llvmFunction.call("variableAssignment", {t->children[0]->children[0]->llvmValue, t->children[1]->llvmValue});
            freeExpressionIfNecessary(t->children[1]);
            return;
        }
        for (size_t i = 0; i < numLHSExpressions; i++) {
            auto LHSExpressionAtomAST = t->children[0]->children[i];
            auto tupleFieldValue = llvmFunction.call("variableGetTupleField", { t->children[1]->llvmValue, ir.getInt64(i + 1) });
            llvmFunction.call("variableAssignment", { LHSExpressionAtomAST->llvmValue, tupleFieldValue });
        }
        freeExpressionIfNecessary(t->children[1]);
    }

    void LLVMGen::visitUnqualifiedType(std::shared_ptr<AST> t) {
        auto runtimeTypeObject = llvmFunction.call("typeMalloc", {});

        std::shared_ptr<MatrixType> matrixType;
        std::shared_ptr<TypedefTypeSymbol> typedefTypeSymbol;
        std::shared_ptr<TupleType> tupleType;
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
            case Type::STRING:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    if (typedefTypeSymbol->type->isMatrixType()) {
                        matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            visit(matrixType->def->children[1]->children[0]);
                            dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                        } else {
                            dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                        }
                        baseType = llvmFunction.call("typeMalloc", {});
                        llvmFunction.call("typeInitFromUnspecifiedString", {baseType});
                        llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                        llvmFunction.call("typeDestructThenFree", baseType);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                        }
                    } else {
                        llvmFunction.call("typeInitFromUnspecifiedString", runtimeTypeObject);
                    }
                } else {
                    if (t->type->isMatrixType()) {
                        matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            visit(matrixType->def->children[1]->children[0]);
                            dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                        } else {
                            dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                        }
                        baseType = llvmFunction.call("typeMalloc", {});
                        llvmFunction.call("typeInitFromUnspecifiedString", {baseType});
                        llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                        llvmFunction.call("typeDestructThenFree", baseType);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                        }
                    } else {
                        llvmFunction.call("typeInitFromUnspecifiedString", runtimeTypeObject);
                    }
                }
                break;
            case Type::BOOLEAN_1:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromBooleanScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            
            case Type::CHARACTER_1:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromCharacterScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            
            case Type::INTEGER_1:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromIntegerScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);

                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            
            case Type::REAL_1:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromRealScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", { runtimeTypeObject, dimension1Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            
            case Type::BOOLEAN_2:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[1]);
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromBooleanScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", { runtimeTypeObject, dimension1Expression, dimension2Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;
            case Type::CHARACTER_2:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[1]);
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromCharacterScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", { runtimeTypeObject, dimension1Expression, dimension2Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;

            case Type::INTEGER_2:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[1]);
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromIntegerScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", { runtimeTypeObject, dimension1Expression, dimension2Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;
            
            case Type::REAL_2:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->type);
                }
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else {
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[1]);
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo());
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromRealScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", { runtimeTypeObject, dimension1Expression, dimension2Expression, baseType });
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;
            
            case Type::TUPLE:
                if (t->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
                    tupleType = std::dynamic_pointer_cast<TupleType>(typedefTypeSymbol->type);
                } else {
                    tupleType = std::dynamic_pointer_cast<TupleType>(t->type);
                }
                auto typeArray = llvmFunction.call("typeArrayMalloc", {ir.getInt64(tupleType->orderedArgs.size())} );
                auto stridArray = llvmFunction.call("stridArrayMalloc", {ir.getInt64(tupleType->orderedArgs.size())} );
                std::shared_ptr<VariableSymbol> argumentSymbol;
                for (size_t i = 0; i < tupleType->orderedArgs.size(); i++) {
                    argumentSymbol = std::dynamic_pointer_cast<VariableSymbol>(tupleType->orderedArgs[i]);
                    visit(argumentSymbol->def);
                    llvmFunction.call("typeArraySet", { typeArray, ir.getInt64(i), argumentSymbol->llvmPointerToTypeObject });
                    if (argumentSymbol->name == "") {
                        llvmFunction.call("stridArraySet", { stridArray, ir.getInt64(i), ir.getInt64(-1) });
                    } else {
                        llvmFunction.call("stridArraySet", { stridArray, ir.getInt64(i), ir.getInt64(symtab->tupleIdentifierAccess.at(argumentSymbol->name)) });
                    }
                }
                llvmFunction.call("typeInitFromTupleType", { runtimeTypeObject, ir.getInt64(tupleType->orderedArgs.size()), typeArray, stridArray });
                
                llvmFunction.call("stridArrayFree", { stridArray });
                for (size_t i = 0; i < tupleType->orderedArgs.size(); i++) {
                    argumentSymbol = std::dynamic_pointer_cast<VariableSymbol>(tupleType->orderedArgs[i]);
                    llvmFunction.call("typeDestructThenFree", argumentSymbol->llvmPointerToTypeObject);
                }
                llvmFunction.call("typeArrayFree", { typeArray });
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

        freeExpressionIfNecessary(t->children[0]);

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
        llvmBranch.hitReturnStat = false;
        visit(t->children[1]);
        if (!llvmBranch.hitReturnStat){
            ir.CreateBr(Merge);
        }
        llvmBranch.hitReturnStat = false;
        // Handle Arbitrary Number of Else If
        int elseIfIdx = 2; // 0 is if expr  1 is if body 2 starts else if
        llvm::BasicBlock* residualElseIfHeader = nullptr; //we declare next else if header in previous itteration because we need it for the conditional branch
        for (auto elseIfStatement : ctx->elseIfStatement()) {
            (void)elseIfStatement;
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
            
            freeExpressionIfNecessary(elifNode->children[0]);
            
            llvm::Value* elseIfCondition = ir.CreateICmpNE(elseIfExprValue, ir.getInt32(0));
            llvm::BasicBlock* elseIfBodyBlock = llvm::BasicBlock::Create(globalCtx, "ElseIfBody", parentFunc);
            // Conditional Branch Out (3 Cases)
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
            llvmBranch.hitReturnStat = false;
            visit(elifNode->children[1]);
            if (!llvmBranch.hitReturnStat){
                ir.CreateBr(Merge);
            }
            llvmBranch.hitReturnStat = false;
            elseIfIdx++; //Elif Counter
        } 
        // Else Caluse (Optional) 
        if (ctx->elseStatement()) {
            parentFunc->getBasicBlockList().push_back(ElseBlock);
            int elseIdx = t->children.size()-1;
            auto elseNode = t->children[elseIdx]->children[0]; 
            ir.SetInsertPoint(ElseBlock);
            llvmBranch.hitReturnStat = false;
            visit(elseNode);
            if (!llvmBranch.hitReturnStat) {
                ir.CreateBr(Merge);
            }
            llvmBranch.hitReturnStat = false;
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
        freeExpressionIfNecessary(t->children[0]);
        
        llvm::Value* condition = ir.CreateICmpNE(exprValue, ir.getInt32(0)); 
        llvmBranch.createPrePredBodyBB(condition);
        visit(t->children[1]);      // Visit body
        llvmBranch.createPrePredMergeBB();

        llvmFunction.call("variableDestructThenFree", { runtimeVarConstZero });
    }

    void LLVMGen::visitPostPredicatedLoop(std::shared_ptr<AST> t) {
        auto runtimeVarConstZero = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromIntegerScalar", {runtimeVarConstZero, ir.getInt32(0)}); //Type must match for ICmpNE 
        llvmBranch.createPostPredBodyBB(); 
        visit(t->children[0]);      //visit Body 
        llvmBranch.createPostPredConditionalBB(); 
        visit(t->children[1]);      //grab value from post predicate
        auto exprValue = llvmFunction.call("variableGetBooleanValue", {t->children[1]->llvmValue});

        freeExpressionIfNecessary(t->children[1]);
        
        llvm::Value *condition = ir.CreateICmpNE(exprValue, ir.getInt32(0));
        llvmBranch.createPostPredMergeBB(condition);

        llvmFunction.call("variableDestructThenFree", { runtimeVarConstZero });
    }
 
    void LLVMGen::visitIteratorLoop(std::shared_ptr<AST> t) { 
        // do with one domain first then generalize
        llvm::Function *parentFunc = ir.GetInsertBlock()->getParent();
        llvm::BasicBlock *preHeader = llvm::BasicBlock::Create(globalCtx, "IteratorLoopPreHeader", parentFunc);
        ir.CreateBr(preHeader);
        ir.SetInsertPoint(preHeader);

        std::vector<llvm::Value*> domainIndexVars;
        std::vector<llvm::Value*> domainExprs;
        std::vector<llvm::Value*> domainExprSizes;
        std::vector<llvm::Value*> domainVars;

        // Constants for all loops
        auto indexVariableType = llvmFunction.call("typeMalloc", {});
        auto constOne = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromIntegerScalar", {constOne, ir.getInt32(1)});
        auto constZero = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromIntegerScalar", {constZero, ir.getInt32(0)});
 
        // Create Preheader and necessary vectors
        for (size_t i = 0; i < t->children.size()-1; i++) {
            // create index variable & set to 0
            auto indexInitialization = llvmFunction.call("variableMalloc", {});
            auto indexVariable = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("typeInitFromIntegerScalar", {indexVariableType});
            llvmFunction.call("variableInitFromIntegerScalar", {indexInitialization, ir.getInt32(0)});
            llvmFunction.call("variableInitFromDeclaration", {indexVariable, indexVariableType, indexInitialization}); 
            llvmFunction.call("variableDestructThenFree", {indexInitialization});
            domainIndexVars.push_back(indexVariable);

            // Initialize domain expressions & push to vector
            visit(t->children[i]);
            auto domainExpr = t->children[i]->children[1];
            auto runtimeDomainArray = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromDomainExpression", {runtimeDomainArray, domainExpr->llvmValue});
            if (domainExpr->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) { //free is not id
                freeExpressionIfNecessary(domainExpr); 
            } 
            domainExprs.push_back(runtimeDomainArray);

            // Calculate size of each domain array and store in vector 
            llvm::Value *length = llvmFunction.call("variableGetLength", {runtimeDomainArray});
            llvm::Value *truncLength = ir.CreateIntCast(length, ir.getInt32Ty(), true);
            auto lengthVariable = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromIntegerScalar", {lengthVariable, truncLength}); 
            domainExprSizes.push_back(lengthVariable);

            //speculative domain variable declaration to satisfy LLVM dominator constraint
            auto domainVar = llvmFunction.call("variableMalloc", {});
            domainVars.push_back(domainVar);
        } 
        // Initialize Basic Blocks 
        for (size_t i = 0; i < t->children.size()-1; i++) { 
            //make basic blocks and set up inserts 
            auto str_i = std::to_string(i);
            llvm::BasicBlock *header = llvm::BasicBlock::Create(globalCtx, "IteratorLoopHeader" + str_i, parentFunc);
            llvm::BasicBlock* body = llvm::BasicBlock::Create(globalCtx, "IteratorLoopBody" + str_i);
            llvm::BasicBlock* merge = llvm::BasicBlock::Create(globalCtx, "IteratorLoopMerge" + str_i);
            llvmBranch.blockStack.push_back(header);
            llvmBranch.blockStack.push_back(body);
            llvmBranch.blockStack.push_back(merge); 
        }
        // Create Header Blocks
        for (size_t i = 0; i < t->children.size()-1; i++) {
            // Headers will cascade from { header_0, header_1 .. header_i } until the final header: header_n  
            size_t bsSize = llvmBranch.blockStack.size();
            llvm::BasicBlock* branchTrue;
            llvm::BasicBlock* branchFalse;
            int offset = (3*(t->children.size()-1-i));
            if (i == 0) {
                ir.CreateBr(llvmBranch.blockStack[bsSize - offset]);
            }
            if (i == (t->children.size() -2)) {
                //header cond br has true: body n (visit t->children[-1] here or false: merge n)
                auto header_n = llvmBranch.blockStack[bsSize - 3];
                auto body_n   = llvmBranch.blockStack[bsSize - 2];
                auto merge_n  = llvmBranch.blockStack[bsSize - 1];
                ir.SetInsertPoint(header_n);
                branchTrue = body_n;
                branchFalse = merge_n;
            } else {
                //header cond br has true: next header (i + 1) or merge i
                auto header_i    = llvmBranch.blockStack[bsSize - offset];
                auto merge_i     = llvmBranch.blockStack[bsSize - offset + 2];
                auto next_header = llvmBranch.blockStack[bsSize - offset + 3];

                ir.SetInsertPoint(header_i);
                //reset the next header array index and domain variable to initial values 
                auto next_loop_index = domainIndexVars[i+1];
                llvmFunction.call("variableAssignment", {next_loop_index, constZero});  
                branchTrue = next_header;
                branchFalse = merge_i;
            }

            //Initialize domain variable
            auto indexVariable = domainIndexVars[i];
            auto runtimeDomainArray = domainExprs[i];
            auto runtimeDomainVar = domainVars[i];
            llvm::Value* index_i32 = llvmFunction.call("variableGetIntegerValue", {indexVariable});
            llvm::Value* index_i64 = ir.CreateIntCast(index_i32, ir.getInt64Ty(), true);
            llvmFunction.call("variableInitFromIntegerArrayElementAtIndex", {runtimeDomainVar, runtimeDomainArray, index_i64});

            //initialize variable symbol to from variable at current index in domain array
            auto variableAST = t->children[i]->children[0];
            auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(variableAST->symbol); 
            variableAST->llvmValue = runtimeDomainVar;
            variableSymbol->llvmPointerToVariableObject = runtimeDomainVar;
            
            //create comparisson between index variable and length of domain vector
            auto comparissonVariable = llvmFunction.call("variableMalloc", {});
            auto lengthVariable = domainExprSizes[i];
            llvmFunction.call("variableInitFromBinaryOp", {comparissonVariable, indexVariable, lengthVariable, ir.getInt32(10)});
            llvm::Value *boolCond = llvmFunction.call("variableGetBooleanValue", {comparissonVariable});
            llvmFunction.call("variableDestructThenFree", {comparissonVariable});
            llvm::Value *branchCond = ir.CreateICmpNE(boolCond, ir.getInt32(0));
            ir.CreateCondBr(branchCond, branchTrue, branchFalse);
        }
        // Create Body and Merge Blocks
        for (int i = t->children.size()-2; i >=0; i--) { 
            size_t bsSize = llvmBranch.blockStack.size();
            int offset = (3*(t->children.size()-1-i));
            auto header_i   = llvmBranch.blockStack[bsSize - offset];
            auto body_i     = llvmBranch.blockStack[bsSize - offset + 1];
            auto merge_i    = llvmBranch.blockStack[bsSize - offset + 2];
            llvm::BasicBlock* next_body; 
            if (i != 0 ) {
                next_body  = llvmBranch.blockStack[bsSize - offset - 2];
            }
            parentFunc->getBasicBlockList().push_back(body_i);
            ir.SetInsertPoint(body_i);

            // Only fill the body on the inner most loop 
            int numChildren = t->children.size();
            if (i == numChildren-2) {
                visit(t->children[numChildren-1]);
            }

            //increment the index variable
            auto indexVariable = domainIndexVars[i];
            auto newIndex = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromBinaryOp", {newIndex, indexVariable, constOne, ir.getInt32(7)});
            llvmFunction.call("variableAssignment", {indexVariable, newIndex});
            llvmFunction.call("variableDestructThenFree", {newIndex}); 

            ir.CreateBr(header_i);
            parentFunc->getBasicBlockList().push_back(merge_i);
            ir.SetInsertPoint(merge_i);
            if (i != 0) {
                ir.CreateBr(next_body);
            }
        }
        // clear the block stack
        for (size_t i = 0; i < 3; i++) {  
            llvmBranch.blockStack.pop_back(); 
        }
        //free memory
        llvmFunction.call("typeDestructThenFree", {indexVariableType});
        llvmFunction.call("variableDestructThenFree", {constZero});
        llvmFunction.call("variableDestructThenFree", {constOne});
        for(size_t i = 0; i < t->children.size()-1; i++) {
            llvmFunction.call("variableDestructThenFree", {domainExprSizes[i]});
            llvmFunction.call("variableDestructThenFree", {domainVars[i]});
            llvmFunction.call("variableDestructThenFree", {domainIndexVars[i]});
            llvmFunction.call("variableDestructThenFree", {domainExprs[i]});
        }
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
        char characterValue;
        if (t->parseTree->getText().length() == 4) {
            switch (t->parseTree->getText()[2]) {
                case 'a':
                    characterValue = '\a';
                    break;
                case 'b':
                    characterValue = '\b';
                    break;
                case 'n':
                    characterValue = '\n';
                    break;
                case 'r':
                    characterValue = '\r';
                    break;
                case 't':
                    characterValue = '\t';
                    break;
                case '\"':
                    characterValue = '\"';
                    break;
                case '\'':
                    characterValue = '\'';
                    break;
                case '\\':
                    characterValue = '\\';
                    break;
            }
        } else {
            characterValue = t->parseTree->getText()[1];
        }
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
        visitChildren(t);
        std::string stringChars = t->parseTree->getText().substr(1, t->parseTree->getText().length() - 2);
        auto stringLength = t->parseTree->getText().length() - 2;
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvm::StringRef string = llvm::StringRef(stringChars.c_str());
        llvm::Value* myStr = ir.CreateGlobalStringPtr(string);
        llvmFunction.call("variableInitFromString", { runtimeVariableObject, ir.getInt64(stringLength), myStr});
        t->llvmValue = runtimeVariableObject;
    }

    void LLVMGen::visitIdentifier(std::shared_ptr<AST> t) {
        visitChildren(t);
        if (numExprAncestors > 0) {
            auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(t->symbol);
            if (variableSymbol != nullptr && variableSymbol->isGlobalVariable) {
                auto globalVar = mod.getNamedGlobal(variableSymbol->name);
                t->llvmValue = ir.CreateLoad(runtimeVariableTy->getPointerTo(), globalVar);
            } else {
                t->llvmValue = t->symbol->llvmPointerToVariableObject;
            }
        }
    }

    void LLVMGen::visitGenerator(std::shared_ptr<AST> t) {
        
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
        llvmFunction.call("typeDestructThenFree", t->children[0]->llvmValue);
        
        freeExpressionIfNecessary(t->children[1]);
    }

    void LLVMGen::visitTypedef(std::shared_ptr<AST> t) {
        return;
    }

    void LLVMGen::visitInputStreamStatement(std::shared_ptr<AST> t) {
        visitChildren(t);
        llvmFunction.call("variableReadFromStdin", { t->children[0]->llvmValue });
    }

    void LLVMGen::visitOutputStreamStatement(std::shared_ptr<AST> t)
    {
        visitChildren(t);
        llvmFunction.call("variablePrintToStdout", {t->children[0]->llvmValue});
        freeExpressionIfNecessary(t->children[0]);
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

        freeExprAtomIfNecessary(t->children[0]);
        freeExprAtomIfNecessary(t->children[1]);
    }

    void LLVMGen::visitUnaryOperation(std::shared_ptr<AST> t) {
        if (t->children[0]->getNodeType() == GazpreaParser::MINUS 
        && t->children[1]->getNodeType() == GazpreaParser::IntegerConstant
        && t->children[1]->parseTree->getText() == "2147483648") {
            // Handle the edge case: integer x = -2147483648;
            auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
            llvmFunction.call("variableInitFromIntegerScalar", {runtimeVariableObject, ir.getInt32(-2147483648)});
            t->llvmValue = runtimeVariableObject;
            return;
        }
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

        freeExprAtomIfNecessary(t->children[1]);
    }

    void LLVMGen::visitIndexing(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        auto numRHSExpressions = t->children[1]->children.size();
        if (numRHSExpressions == 1) {
            llvmFunction.call(
                "variableInitFromVectorIndexing", 
                { 
                    runtimeVariableObject, 
                    t->children[0]->llvmValue, 
                    t->children[1]->children[0]->llvmValue 
                }
            );
            freeExpressionIfNecessary(t->children[1]->children[0]);
        } else {
            llvmFunction.call(
                "variableInitFromMatrixIndexing", 
                { 
                    runtimeVariableObject, 
                    t->children[0]->llvmValue, 
                    t->children[1]->children[0]->llvmValue, 
                    t->children[1]->children[1]->llvmValue 
                }
            );
            freeExpressionIfNecessary(t->children[1]->children[0]);
            freeExpressionIfNecessary(t->children[1]->children[1]);
        }
        t->llvmValue = runtimeVariableObject;
        freeExprAtomIfNecessary(t->children[0]);
    }

    void LLVMGen::visitInterval(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromBinaryOp", {runtimeVariableObject, t->children[0]->llvmValue, t->children[1]->llvmValue, ir.getInt32(1)});
        t->llvmValue = runtimeVariableObject;

        freeExprAtomIfNecessary(t->children[0]);
        freeExprAtomIfNecessary(t->children[1]);
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
        auto *ctx = dynamic_cast<GazpreaParser::CallProcedureFunctionInExpressionContext*>(t->parseTree);
        //Exception for misaligned argument pass  
        int numArgsExpected = subroutineSymbol->declaration->children[1]->children.size(); 
        int numArgsRecieved = t->children[1]->children.size(); 
        if (numArgsExpected != numArgsRecieved) {
            throw InvalidArgumentError(subroutineSymbol->declaration->children[0]->getText(), t->getText(),
                ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine()
            );
        }       
        std::vector<llvm::Value *> arguments = std::vector<llvm::Value *>();
        if (!t->children[1]->isNil()) {
            for (auto expressionAST : t->children[1]->children){
                arguments.push_back(expressionAST->llvmValue);
            }
        }
        t->llvmValue = ir.CreateCall(subroutineSymbol->llvmFunction, arguments);
        if (!t->children[1]->isNil()) {
            for (auto expressionAST : t->children[1]->children) {
                freeExpressionIfNecessary(expressionAST);
            }
        }
    }

    void LLVMGen::visitCallSubroutineStatement(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->children[0]->symbol);
        auto *ctx = dynamic_cast<GazpreaParser::CallProcedureContext*>(t->parseTree);
        //Throw exception for invalid arguments 
        int numArgsExpected = subroutineSymbol->declaration->children[1]->children.size(); 
        int numArgsRecieved = t->children[1]->children.size(); 
        if (numArgsExpected != numArgsRecieved) {
            throw InvalidArgumentError(subroutineSymbol->declaration->children[0]->getText(), t->getText(),
                ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine()
            );
        }  
        std::vector<llvm::Value *> arguments = std::vector<llvm::Value *>();
        if (!t->children[1]->isNil()) {
            for (auto expressionAST : t->children[1]->children) {
                arguments.push_back(expressionAST->llvmValue);
            }
        }
        if (subroutineSymbol->type == nullptr) {
            ir.CreateCall(subroutineSymbol->llvmFunction, arguments);
        } else {
            auto returnValue = ir.CreateCall(subroutineSymbol->llvmFunction, arguments);
            llvmFunction.call("variableDestructThenFree", returnValue);
        }

        if (!t->children[1]->isNil()) {
            for (auto expressionAST : t->children[1]->children) {
                freeExpressionIfNecessary(expressionAST);
            }
        }
    }

    void LLVMGen::visitParameterAtom(std::shared_ptr<AST> t) {
        auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(t->symbol);
        auto runtimeTypeObject = llvmFunction.call("typeMalloc", {}); 
        std::shared_ptr<MatrixType> matrixType;
        std::shared_ptr<TupleType> tupleType;
        std::shared_ptr<TypedefTypeSymbol> typedefTypeSymbol;
        llvm::Value *baseType;
        llvm::Value *dimension1Expression = nullptr;
        llvm::Value *dimension2Expression = nullptr;

        if (variableSymbol->type == nullptr) {
            // inferred type qualifier
            llvmFunction.call("typeInitFromUnknownType", { runtimeTypeObject });
            variableSymbol->llvmPointerToTypeObject = runtimeTypeObject;
            return;
        }
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
            case Type::STRING:
                if (t->symbol->type->isTypedefType()) {
                    auto typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    if (typedefTypeSymbol->type->isMatrixType()) {
                        matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            visit(matrixType->def->children[1]->children[0]);
                            dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                        } else { 
                            dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                        }
                        baseType = llvmFunction.call("typeMalloc", {});
                        llvmFunction.call("typeInitFromUnspecifiedString", {baseType});
                        llvmFunction.call("typeInitFromVectorSizeSpecification", {runtimeTypeObject, dimension1Expression, baseType});
                        llvmFunction.call("typeDestructThenFree", baseType);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                        }
                    } else {
                        llvmFunction.call("typeInitFromUnspecifiedString", runtimeTypeObject);
                    }
                } else {
                    if (t->symbol->type->isMatrixType()) {
                        matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            visit(matrixType->def->children[1]->children[0]);
                            dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                        } else { 
                            dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                        }
                        baseType = llvmFunction.call("typeMalloc", {});
                        llvmFunction.call("typeInitFromUnspecifiedString", {baseType});
                        llvmFunction.call("typeInitFromVectorSizeSpecification", {runtimeTypeObject, dimension1Expression, baseType});
                        llvmFunction.call("typeDestructThenFree", baseType);
                        if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                            freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                        }
                    } else {
                        llvmFunction.call("typeInitFromUnspecifiedString", runtimeTypeObject);
                    }
                }
                break;
            case Type::BOOLEAN_1: {
                if (t->symbol->type->isTypedefType()) {
                    auto typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                }    
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else { 
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromBooleanScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", {runtimeTypeObject, dimension1Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            }
            case Type::CHARACTER_1: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                }    
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                        visit(matrixType->def->children[1]->children[0]);
                        dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else { 
                        dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromCharacterScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", {runtimeTypeObject, dimension1Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            }
            case Type::INTEGER_1: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                }    
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                        visit(matrixType->def->children[1]->children[0]);
                        dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else { 
                        dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromIntegerScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", {runtimeTypeObject, dimension1Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            }
            case Type::REAL_1: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                }    
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                        visit(matrixType->def->children[1]->children[0]);
                        dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else { 
                        dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromRealScalar", {baseType});
                llvmFunction.call("typeInitFromVectorSizeSpecification", {runtimeTypeObject, dimension1Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                break;
            }
            case Type::BOOLEAN_2: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                } if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                        visit(matrixType->def->children[1]->children[0]);
                        dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else { 
                        dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                } if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                        visit(matrixType->def->children[1]->children[1]);
                        dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                        dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromBooleanScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", {runtimeTypeObject, dimension1Expression, dimension2Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;
            }
            case Type::CHARACTER_2: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                } if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else { 
                        dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                } if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[1]);
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromCharacterScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", {runtimeTypeObject, dimension1Expression, dimension2Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;
            }
            case Type::INTEGER_2: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                } if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[0]->llvmValue;
                } else { 
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                } if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[1]);
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromIntegerScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", {runtimeTypeObject, dimension1Expression, dimension2Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;
            }
            case Type::REAL_2: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    matrixType = std::dynamic_pointer_cast<MatrixType>(typedefTypeSymbol->type);
                } else {
                    matrixType = std::dynamic_pointer_cast<MatrixType>(t->symbol->type);
                }  if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[0]);
                    dimension1Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else { 
                    dimension1Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                } if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    visit(matrixType->def->children[1]->children[1]);
                    dimension2Expression = matrixType->def->children[1]->children[1]->llvmValue;
                } else {
                    dimension2Expression = llvm::Constant::getNullValue(runtimeVariableTy->getPointerTo()); //vector size unknown at compile time
                }
                baseType = llvmFunction.call("typeMalloc", {});
                llvmFunction.call("typeInitFromRealScalar", {baseType});
                llvmFunction.call("typeInitFromMatrixSizeSpecification", {runtimeTypeObject, dimension1Expression, dimension2Expression, baseType});
                llvmFunction.call("typeDestructThenFree", baseType);
                if (matrixType->def->children[1]->children[0]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[0]);
                }
                if (matrixType->def->children[1]->children[1]->getNodeType() == GazpreaParser::EXPRESSION_TOKEN) {
                    freeExpressionIfNecessary(matrixType->def->children[1]->children[1]);
                }
                break;
            }
            case Type::TUPLE: {
                if (t->symbol->type->isTypedefType()) {
                    typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol->type);
                    tupleType = std::dynamic_pointer_cast<TupleType>(typedefTypeSymbol->type);
                } else {
                    tupleType = std::dynamic_pointer_cast<TupleType>(t->symbol->type);
                }
                auto typeArray = llvmFunction.call("typeArrayMalloc", {ir.getInt64(tupleType->orderedArgs.size())} );
                auto stridArray = llvmFunction.call("stridArrayMalloc", {ir.getInt64(tupleType->orderedArgs.size())} );
                std::shared_ptr<VariableSymbol> argumentSymbol;
                for (size_t i = 0; i < tupleType->orderedArgs.size(); i++) {
                    argumentSymbol = std::dynamic_pointer_cast<VariableSymbol>(tupleType->orderedArgs[i]);
                    visit(argumentSymbol->def);
                    llvmFunction.call("typeArraySet", { typeArray, ir.getInt64(i), argumentSymbol->llvmPointerToTypeObject });
                    if (argumentSymbol->name == "") {
                        llvmFunction.call("stridArraySet", { stridArray, ir.getInt64(i), ir.getInt64(-1) });
                    } else {
                        llvmFunction.call("stridArraySet", { stridArray, ir.getInt64(i), ir.getInt64(symtab->tupleIdentifierAccess.at(argumentSymbol->name)) });
                    }
                }
                llvmFunction.call("typeInitFromTupleType", { runtimeTypeObject, ir.getInt64(tupleType->orderedArgs.size()), typeArray, stridArray });
                llvmFunction.call("stridArrayFree", { stridArray });

                for (size_t i = 0; i < tupleType->orderedArgs.size(); i++) {
                    argumentSymbol = std::dynamic_pointer_cast<VariableSymbol>(tupleType->orderedArgs[i]);
                    llvmFunction.call("typeDestructThenFree", argumentSymbol->llvmPointerToTypeObject);
                }
                llvmFunction.call("typeArrayFree", { typeArray });
            }
        }
        variableSymbol->llvmPointerToTypeObject = runtimeTypeObject;
    }

    void LLVMGen::visitTupleLiteral(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto numExpressions = t->children[0]->children.size();
        auto runtimeVariableArray = llvmFunction.call("variableArrayMalloc", { ir.getInt64(numExpressions) });
        for (size_t i = 0; i < numExpressions; i++) {
            llvmFunction.call("variableArraySet", { runtimeVariableArray, ir.getInt64(i), t->children[0]->children[i]->llvmValue });
        }
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromTupleLiteral", { runtimeVariableObject, ir.getInt64(numExpressions), runtimeVariableArray });
        t->llvmValue = runtimeVariableObject;
        llvmFunction.call("variableArrayFree", { runtimeVariableArray });

        // Free all unused variables
        for (size_t i = 0; i < numExpressions; i++) {
            freeExpressionIfNecessary(t->children[0]->children[i]);
        }
    }

    void LLVMGen::visitVectorMatrixLiteral(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto numExpressions = t->children[0]->children.size();
        auto runtimeVariableArray = llvmFunction.call("variableArrayMalloc", { ir.getInt64(numExpressions) });
        for (size_t i = 0; i < numExpressions; i++) {
            llvmFunction.call("variableArraySet", { runtimeVariableArray, ir.getInt64(i), t->children[0]->children[i]->llvmValue });
        }
        auto runtimeVariableObject = llvmFunction.call("variableMalloc", {});
        llvmFunction.call("variableInitFromVectorLiteral", { runtimeVariableObject, ir.getInt64(numExpressions), runtimeVariableArray });
        
        t->llvmValue = runtimeVariableObject;
        llvmFunction.call("variableArrayFree", { runtimeVariableArray });

        // Free all unused variables
        for (size_t i = 0; i < numExpressions; i++) {
            freeExpressionIfNecessary(t->children[0]->children[i]);
        }
    }

    void LLVMGen::visitTupleAccess(std::shared_ptr<AST> t) {
        visit(t->children[0]);
        if (t->children[1]->getNodeType() == GazpreaParser::IDENTIFIER_TOKEN) {
            auto tupleType = std::dynamic_pointer_cast<TupleType>(t->children[0]->evalType);
            auto identifierName = t->children[1]->parseTree->getText();
            if (tupleType != nullptr) {
                size_t i;
                for (i = 0; i < tupleType->orderedArgs.size(); i++) {
                    if (tupleType->orderedArgs[i]->name == identifierName) {
                        break;
                    }
                }
                t->llvmValue = llvmFunction.call("variableGetTupleField", { t->children[0]->llvmValue, ir.getInt64(i + 1) });
            } else {
                t->llvmValue = llvmFunction.call("variableGetTupleFieldFromID", { t->children[0]->llvmValue, ir.getInt64(symtab->tupleIdentifierAccess.at(identifierName)) });
            }
        } else {
            auto index = std::stoi(t->children[1]->parseTree->getText());
            t->llvmValue = llvmFunction.call("variableGetTupleField", { t->children[0]->llvmValue, ir.getInt64(index) }); 
        }
    }

    void LLVMGen::visitBlock(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto localScope = std::dynamic_pointer_cast<LocalScope>(t->scope);
        if (!localScope->containReturn) {
            freeAllVariablesDeclaredInBlockScope(localScope);
        }
    }

    void LLVMGen::initializeGlobalVariables() {
        // Initialize global variables (should only call in the beginning of main())
        for (auto variableSymbol : symtab->globals->globalVariableSymbols) {
            auto globalVar = mod.getNamedGlobal(variableSymbol->name);
            visit(variableSymbol->def->children[2]);
            ir.CreateStore(variableSymbol->def->children[2]->llvmValue, globalVar);
        }
    }

    void LLVMGen::freeGlobalVariables() {
        // Free all global variables
        for (auto variableSymbol : symtab->globals->globalVariableSymbols) {
            auto globalVarAddress = mod.getNamedGlobal(variableSymbol->name);
            auto globalVar = ir.CreateLoad(runtimeVariableTy->getPointerTo(), globalVarAddress);
            llvmFunction.call("variableDestructThenFree", globalVar);
        }
    }

    void LLVMGen::freeAllVariablesDeclaredInBlockScope(std::shared_ptr<LocalScope> scope) {
        // Free all VariableSymbol in the Scope
        for (auto const& [key, val] : scope->symbols) {
            auto vs = std::dynamic_pointer_cast<VariableSymbol>(val);
            if (vs != nullptr && vs->llvmPointerToVariableObject != nullptr) {
                llvmFunction.call("variableDestructThenFree", vs->llvmPointerToVariableObject);
            }
        }
    }

    void LLVMGen::initializeSubroutineParameters(std::shared_ptr<SubroutineSymbol> subroutineSymbol) {
        for (size_t i = 0; i < subroutineSymbol->orderedArgs.size(); i++) {
            auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(subroutineSymbol->orderedArgs[i]);
            if (variableSymbol->typeQualifier == "const") {
                auto runtimeVariableParameterObject = llvmFunction.call("variableMalloc", {});
                llvmFunction.call(
                    "variableInitFromParameter", 
                    {
                        runtimeVariableParameterObject, 
                        variableSymbol->llvmPointerToTypeObject, 
                        subroutineSymbol->llvmFunction->getArg(i) 
                    }
                );
                variableSymbol->llvmPointerToVariableObject = runtimeVariableParameterObject;
                llvmFunction.call("typeDestructThenFree", variableSymbol->llvmPointerToTypeObject);
            } else {
                variableSymbol->llvmPointerToVariableObject = subroutineSymbol->llvmFunction->getArg(i);
                if (variableSymbol->type != nullptr && variableSymbol->type->getTypeId() == Type::TUPLE) {
                    // Only swap/redefine type if the parameter type is tuple
                    auto oldType = llvmFunction.call(
                        "variableSwapType", 
                        { variableSymbol->llvmPointerToVariableObject, variableSymbol->llvmPointerToTypeObject }
                    );
                    subroutineSymbol->oldParameterTypes.push_back(oldType);
                } else {
                    subroutineSymbol->oldParameterTypes.push_back(nullptr);
                    llvmFunction.call("typeDestructThenFree", variableSymbol->llvmPointerToTypeObject);
                }
            }
        }
    }

    void LLVMGen::freeSubroutineParameters(std::shared_ptr<SubroutineSymbol> subroutineSymbol) {
        // Free all variables from variableInitFromParameter()
        for (size_t i = 0; i < subroutineSymbol->orderedArgs.size(); i++) {
            auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(subroutineSymbol->orderedArgs[i]);
            if (variableSymbol->typeQualifier == "const") {
                llvmFunction.call("variableDestructThenFree", variableSymbol->llvmPointerToVariableObject);
            } else {
                if (subroutineSymbol->oldParameterTypes[i] != nullptr) {
                    auto subroutineParameterType = llvmFunction.call(
                        "variableSwapType", 
                        { variableSymbol->llvmPointerToVariableObject, subroutineSymbol->oldParameterTypes[i] }
                    );
                    llvmFunction.call("typeDestructThenFree", subroutineParameterType);
                }
            }
        }
    }

    void LLVMGen::freeExpressionIfNecessary(std::shared_ptr<AST> t) {
        if (t->children[0]->getNodeType() != GazpreaParser::IDENTIFIER_TOKEN
        && t->children[0]->getNodeType() != GazpreaParser::TUPLE_ACCESS_TOKEN) {
            llvmFunction.call("variableDestructThenFree", t->llvmValue);
        }
    }

    void LLVMGen::freeExprAtomIfNecessary(std::shared_ptr<AST> t) {
        if (t->getNodeType() != GazpreaParser::IDENTIFIER_TOKEN
        && t->getNodeType() != GazpreaParser::TUPLE_ACCESS_TOKEN) {
            llvmFunction.call("variableDestructThenFree", t->llvmValue);
        }
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