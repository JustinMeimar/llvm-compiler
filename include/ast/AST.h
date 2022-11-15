#pragma once
#include "llvm/IR/Module.h"
#include "antlr4-runtime.h"
#include "GazpreaParser.h"
#include "GazpreaLexer.h"

#include "BaseErrorListener.h"

#include "Type.h"
#include "Scope.h"
#include "Symbol.h"

#include <vector>
#include <string>
#include <memory>

namespace gazprea {
    class Scope;   // forward declaration of Scope to resolve circular dependency
    class Symbol;  // forward declaration of Scope to resolve circular dependency

    class AST { // Homogeneous AST node type
    public:
		//Static members
        const static size_t NIL_TYPE = 65535;  // distinguish from other numbers (since size_t is unsigned I can't use -1)
        antlr4::tree::ParseTree *parseTree;       // From which parse tree node did we create node?
        size_t nodeType;                            // type of the AST node
        std::vector<std::shared_ptr<AST>> children; // normalized list of children
      	
		//LLVM Codegen
		llvm::Value* llvmValue;

		//TypeWalk
		std::shared_ptr<Type> evalType;
		std::shared_ptr<Type> promoteToType;
		std::vector<std::shared_ptr<Type>> tuplePromoteTypeList;

        std::shared_ptr<Symbol> symbol; // Populate by Def and Ref pass
        std::shared_ptr<Type> type;  // Only use for visitUnqualifiedType() and its subrules in Ref pass
        std::shared_ptr<Scope> scope;  // Populate by Def pass

		//Methods	
		virtual ~AST();	
    	static std::shared_ptr<AST> NewNilNode(); /** create a node with NIL_TYPE and no parse tree node */
    	explicit AST(antlr4::tree::ParseTree *parseTree); /** create a node with a parse tree */	
    	explicit AST(size_t tokenType, antlr4::tree::ParseTree *parseTree = nullptr); /** Create node from token type; used mainly for imaginary tokens */	
    	size_t getNodeType(); /** External visitors execute the same action for all nodes*  with same node type while walking. */
    
    	void addChild(std::any t);
    	void addChild(const std::shared_ptr<AST>& t);
    	bool isNil(); /** returns true if and only if the node is created with a NIL_TYPE and no parse tree node is given */
    	std::string getText(); /** get the Gazprea source code text for a node */

    	// ------------ FOR DEBUGGING ONLY -------------	
    	std::string toString(gazprea::GazpreaParser *parser); /** Compute string for single node */	
    	std::string toStringTree(gazprea::GazpreaParser *parser); /** Compute string for a whole tree not just a node */
    };
}