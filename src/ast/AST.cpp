#include "AST.h"

#include <sstream>

AST::AST(antlr4::tree::ParseTree *parseTree) :parseTree(parseTree), nodeType(NIL_TYPE) {}
AST::AST(size_t tokenType, antlr4::tree::ParseTree *parseTree) :parseTree(parseTree), nodeType(tokenType) {}

size_t AST::getNodeType() { return nodeType; }

void AST::addChild(std::any t) {
    this->addChild(std::any_cast<std::shared_ptr<AST>>(t)); // There is only one valid type for t. Pass it to AST::addChild(std::shared_ptr<AST> t)
}
void AST::addChild(const std::shared_ptr<AST>& t) {
    children.push_back(t);
}
bool AST::isNil() { return parseTree == nullptr && nodeType == NIL_TYPE; }

std::string AST::toString(gazprea::GazpreaParser *parser) {
    std::string result;
    bool rawText = false;
    if (parseTree != nullptr) {
        if (!children.empty()) {
            auto *ctx = dynamic_cast<antlr4::RuleContext *>(parseTree);
            result = parser->getRuleNames()[ctx->getRuleIndex()];  // use rule name
        } else {
            result = "\"" + parseTree->getText() + "\"";  // use terminal (ast) node text
            rawText = true;
        }
    }
    if (nodeType != NIL_TYPE) {
        std::string astTypeName {parser->getVocabulary().getSymbolicName(nodeType)};
        if (result.empty()) {
            result = astTypeName;
        } else if (rawText) {
            result = astTypeName + ":" + result;
        } else {
            result = astTypeName;
        }
    }
    if (result.empty()) {
        result = "Nil";
    }
    return result;
}

std::string AST::toStringTree(gazprea::GazpreaParser *parser) {
    if ( children.empty() ) return toString(parser);
    std::stringstream buf;
    if ( !isNil() ) {
        std::cout << '(' << toString(parser) << ' ';
    }
    for (auto iter = children.begin(); iter != children.end(); iter++) {
        std::shared_ptr<AST> t = *iter; // normalized (unnamed) children
        if ( iter != children.begin() ) std::cout << ' ';
        std::cout << t->toStringTree(parser);
    }
    if ( !isNil() ) std::cout << ')';
    return buf.str();
}

AST::~AST() {}

std::shared_ptr<AST> AST::NewNilNode() {
    return std::make_shared<AST>(nullptr);
}

std::string AST::getText() {
    return parseTree->getText();
}
