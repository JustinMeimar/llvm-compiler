#pragma once

#include "antlr4-runtime.h"
#include "GazpreaParser.h"
#include "GazpreaLexer.h"

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
        const static size_t NIL_TYPE = 65535;  // distinguish from other numbers (since size_t is unsigned I can't use -1)
        static std::shared_ptr<AST> createNil();

        antlr4::tree::ParseTree *parseTree;       // From which parse tree node did we create node?
        size_t nodeType;                            // type of the AST node
        std::vector<std::shared_ptr<AST>> children; // normalized list of children


    	/** create a node with NIL_TYPE and no parse tree node */
    	static std::shared_ptr<AST> NewNilNode();
    	/** create a node with a parse tree */
    	explicit AST(antlr4::tree::ParseTree *parseTree);
    	/** Create node from token type; used mainly for imaginary tokens */
    	explicit AST(size_t tokenType, antlr4::tree::ParseTree *parseTree = nullptr);

   	 	/** External visitors execute the same action for all nodes
    	 *  with same node type while walking. */
    	size_t getNodeType();
    
    	void addChild(std::any t);
    	void addChild(const std::shared_ptr<AST>& t);

    	/** returns true if and only if the node is created with a NIL_TYPE and no parse tree node is given */
    	bool isNil();

    	/** get the Gazprea source code text for a node */
    	std::string getText();

    	// ------------ FOR DEBUGGING ONLY -------------
    	/** Compute string for single node */
    	std::string toString(gazprea::GazpreaParser *parser);
    	/** Compute string for a whole tree not just a node */
    	std::string toStringTree(gazprea::GazpreaParser *parser);

        virtual ~AST();

        std::shared_ptr<Symbol> symbol; // Populate by Def and Ref pass
        std::shared_ptr<Type> type;  // Solely use for Type Reference in Ref pass
        std::shared_ptr<Scope> scope;  // Populate by Def pass
    };
}