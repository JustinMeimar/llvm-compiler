#include "GazpreaLexer.h"
#include "GazpreaParser.h"
#include "ANTLRFileStream.h"
#include "CommonTokenStream.h"
#include "tree/ParseTree.h"
#include "tree/ParseTreeWalker.h"
#include "AST.h"
#include "SymbolTable.h"
#include "ASTBuilder.h"
#include "DefWalk.h"
#include "RefWalk.h"
#include "TypeWalk.h"
#include "LLVMGen.h"
#include "DiagnosticErrorListener.h"
#include "BailErrorStrategy.h"
#include "exceptions.h"
#include <iostream>
#include <fstream>

class MyErrorListener : public antlr4::BaseErrorListener {
    
    void syntaxError(antlr4::Recognizer *recognizer, antlr4::Token * offendingSymbol, size_t line, size_t charPositionInLine, const std::string &msg, std::exception_ptr e) override {
        std::vector<std::string> rule_stack = ((antlr4::Parser*) recognizer)->getRuleInvocationStack();
        std::string newMsg = "Synatax error in rule: " + rule_stack[0] + " " + std::to_string(line) + ":" + std::to_string(charPositionInLine);
        throw gazprea::SyntaxError(newMsg); 
    }
};

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "Missing required argument.\n"
              << "Required arguments: <input file path> <output file path>\n";
    return 1;
  }

  // Open the file then parse and lex it.
  antlr4::ANTLRFileStream afs;
  afs.loadFromFile(argv[1]);
  gazprea::GazpreaLexer lexer(&afs);
  antlr4::CommonTokenStream tokens(&lexer);
  gazprea::GazpreaParser parser(&tokens);

  parser.removeErrorListeners(); // Remove the default console error listener
  parser.addErrorListener(new MyErrorListener()); // Add our error listener
  
  // Get the root of the parse tree. Use your base rule name.
  antlr4::tree::ParseTree *tree = parser.compilationUnit();
  // std::cout << tree->toStringTree(&parser, true) << std::endl;  // pretty print parse tree
  
  //Build AST
  gazprea::ASTBuilder builder;
  auto ast = std::any_cast<std::shared_ptr<gazprea::AST>>(builder.visit(tree));

  // Initialize the symbol table
  std::string outfile(argv[2]);
  auto symtab = std::make_shared<gazprea::SymbolTable>();

  gazprea::DefWalk defwalk(symtab);
  defwalk.visit(ast);
  if(!defwalk.hasMainProcedure) {
    throw gazprea::MissingMainProcedureError("main");
  }

  gazprea::RefWalk refwalk(symtab);
  refwalk.visit(ast);

  gazprea::TypeWalk typewalk(symtab);
  typewalk.visit(ast);

  gazprea::LLVMGen llvmgen(symtab, outfile);
  llvmgen.visit(ast);
  
  return 0;
}
