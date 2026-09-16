#include "./include/core/arena.hpp"
#include "./include/core/option.hpp"
#include "./include/core/result.hpp"
#include "include/ast/ast.hpp"
#include "include/ast/astPrinter.hpp"
#include "include/core/cmdparser.hpp"
#include "include/core/macros.hpp"
#include "include/core/types.hpp"
#include "include/lexer/interner.hpp"
#include "include/lexer/lexer.hpp"
#include "include/lexer/line_index.hpp"
#include "include/lexer/span.hpp"
#include "include/lexer/token.hpp"
#include "include/parser/parser.hpp"
#include "include/sema/sema.hpp"

#include <cstdio>
#include <iostream>
#include <sys/types.h>

auto main(int argc, char **argv) -> i32 {

  /* CORE */
  CmdParser cmdParse(argc, argv);
  auto file = cmdParse.file();

  /* LEXER */
  LineIndex lineIndex(file);
  Interner interner{};

  Lexer lexer(file, interner);
  LexerState s0{0};

  // auto step = lexer.next_token(s0);
  // step = lexer.next_token(s0);
  // while(step.value().token.kind != TokenKind::TK_EoF){
  //   std::printf("[%u:%u] Token: %s\n", step.value().token.span.loc.line,
  //   step.value().token.span.loc.col,
  //   token_kind_to_str(step.value().token.kind));

  //   if(step.value().token.kind == TokenKind::TK_ID || step.value().token.kind
  //   == TokenKind::TK_STR_LITERAL){
  //     strview sv = interner.view(step.value().token.symbol_id);
  //     std::cout << sv << "\n";
  //   }

  //   step = lexer.next_token(step.value().next_state);
  // }


  Arena arena;

  Parser parser(lexer, arena, &interner);
  Node *nodes = parser.parse_prog();

  ASTPrinter astPrinter(interner);
  astPrinter.print(nodes);

  Sema sema(&interner, nodes);
  if(!sema.analyze()){
    PANICF("something went wrong in sema");
  }

  
  return 0;
}
