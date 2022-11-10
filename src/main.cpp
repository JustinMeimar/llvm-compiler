#include "GazpreaLexer.h"
#include "GazpreaParser.h"

#include "ANTLRFileStream.h"
#include "CommonTokenStream.h"
#include "tree/ParseTree.h"
#include "tree/ParseTreeWalker.h"

#include "AST.h"
#include "ASTBuilder.h"
#include "DefWalk.h"
#include "RefWalk.h"
#include "TypeWalk.h"
#include "SymbolTable.h"

#include "DiagnosticErrorListener.h"
#include "BailErrorStrategy.h"

#include <iostream>
#include <fstream>

void setParserReportAllErrors(gazprea::GazpreaParser &parser) {
    std::shared_ptr<antlr4::BailErrorStrategy> handler = std::make_shared<antlr4::BailErrorStrategy>();
    parser.setErrorHandler(handler);
//    auto * listener = new antlr4::DiagnosticErrorListener();
//    parser.addErrorListener(listener);
//    parser.getInterpreter<antlr4::atn::ParserATNSimulator>()->setPredictionMode(antlr4::atn::PredictionMode::LL_EXACT_AMBIG_DETECTION);
}

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

  setParserReportAllErrors(parser);

  // Get the root of the parse tree. Use your base rule name.
  antlr4::tree::ParseTree *tree = parser.compilationUnit();
  // std::cout << tree->toStringTree(&parser, true) << std::endl;  // pretty print parse tree
  
  //Build AST
  gazprea::ASTBuilder builder;
  auto ast = std::any_cast<std::shared_ptr<gazprea::AST>>(builder.visit(tree));

  // Initialize the symbol table
  auto symtab = std::make_shared<gazprea::SymbolTable>();

  gazprea::DefWalk defwalk(symtab);
  defwalk.visit(ast);

  gazprea::RefWalk refwalk(symtab);
  refwalk.visit(ast);

  gazprea::TypeWalk typewalk(symtab);
  typewalk.visit(ast);

  // std::cout << "ast:\n" << ast->toStringTree(&parser) << std::endl;

  return 0;
}
