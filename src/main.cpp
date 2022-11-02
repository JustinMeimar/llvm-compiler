#include "GazpreaLexer.h"
#include "GazpreaParser.h"

#include "ANTLRFileStream.h"
#include "CommonTokenStream.h"
#include "tree/ParseTree.h"
#include "tree/ParseTreeWalker.h"

#include "AST.h"
#include "ASTBuilder.h"
#include "DefWalk.h"

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
  
  //Build AST
  gazprea::ASTBuilder builder;
  std::shared_ptr<AST> ast = std::any_cast<std::shared_ptr<AST>>(builder.visit(tree));

  gazprea::DefWalk defwalk;
  defwalk.visit(ast);

  // std::cout << tree->toStringTree(&parser, true) << std::endl;  // pretty print parse tree
  // std::cout << "ast:\n" << ast->toStringTree() << std::endl;

  return 0;
}
