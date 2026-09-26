#include "./include/core/arena.hpp"
#include "./include/core/option.hpp"
#include "./include/core/result.hpp"
#include "include/ast/ast.hpp"
#include "include/ast/astPrinter.hpp"
#include "include/core/cmdparser.hpp"
#include "include/core/macros.hpp"
#include "include/core/types.hpp"
#include "include/diagnostic/diag.hpp"
#include "include/lexer/interner.hpp"
#include "include/lexer/lexer.hpp"
#include "include/lexer/line_index.hpp"
#include "include/lexer/span.hpp"
#include "include/lexer/token.hpp"
#include "include/parser/parser.hpp"
#include "include/sema/sema.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <sys/types.h>

auto main(int argc, char **argv) -> i32 {

  /* CORE */
  CmdParser cmdParse(argc, argv);
  cmdParse.parse();
  auto file = cmdParse.file();
  auto filename = cmdParse.filename();

  // std::ifstream ifs("main.rue");
  // std::ostringstream oss; oss << ifs.rdbuf();
  // auto file = oss.str();
  // auto filename = "main.rue";

  /* DIAGNOSTICS */
  Diag diag(filename, file);

  /* LEXER */
  LineIndex lineIndex(file);
  Interner interner{};

  Lexer lexer(file, interner, diag);
  LexerState s0{0};

  Arena arena;

  Parser parser(lexer, arena, &interner, diag);
  Node *nodes = parser.parse_prog();
  if (diag.has_error()) {
    diag.render_all();
    return EXIT_FAILURE;
  }

  ASTPrinter astPrinter(interner);
  if (cmdParse.display_ast())
    astPrinter.print(nodes);

  Sema sema(&interner, nodes, diag);
  if (!sema.analyze() && diag.has_error()) {
    diag.render_all();
    return EXIT_FAILURE;
  }

  // if(!sema.analyze()){
  //   PANICF("something went wrong in sema");
  // }

  arena.destory_all();

  return 0;
}
