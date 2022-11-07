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
        const static size_t NIL_TYPE = 65535;
        static std::shared_ptr<AST> createNil();

        antlr4::tree::ParseTree *parseTree;       // From which parse tree node did we create node?
        size_t nodeType;                            // type of the AST node
        std::vector<std::shared_ptr<AST>> children; // normalized list of children

        explicit AST(antlr4::tree::ParseTree *parseTree);
        /** Create node from token type; used mainly for imaginary tokens */
        explicit AST(size_t tokenType);
        AST(size_t tokenType, antlr4::tree::ParseTree *parseTree);

        /** External visitors execute the same action for all nodes
         *  with same node type while walking. */
        size_t getNodeType();
        
        void addChild(std::any t);
        void addChild(const std::shared_ptr<AST>& t);
        bool isNil();

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