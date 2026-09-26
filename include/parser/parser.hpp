#pragma once

#include "../ast/ast.hpp"
#include "../lexer/lexer.hpp"
#include "../diagnostic/diag.hpp"
#include "../core/option.hpp"

// TODO: make it take an err_msg
#define EXPECT_NORETURN(tokenKind)                                             \
  Token CONCAT_IMPL(tok, __LINE__) = consume_token();                          \
  if (CONCAT_IMPL(tok, __LINE__).kind != tokenKind) {                          \
    diag_.push_diag(                                                            \
        {SeverityKind::Err, peek().span,                                          \
         "expected token '" + (string)token_kind_to_str(tokenKind) +           \
             "' got '" +                                                       \
             (string)token_kind_to_str(CONCAT_IMPL(tok, __LINE__).kind) +      \
             "'"});                                                            \
  }

#define EXPECT_RETURN(tokenKind)                                               \
  ({                                                                           \
    Token tok = consume_token();                                               \
    if (tok.kind != tokenKind) {                                               \
      diag_.push_diag(                                                          \
          {SeverityKind::Err, peek().span,                                        \
           "expected token '" + (string)token_kind_to_str(tokenKind) +         \
               "' got '" +                                                     \
               (string)token_kind_to_str(tok.kind) +    \
               "'"});                                                          \
    }                                                                          \
    tok;                                                                       \
  });

#define _PREC_                                                                 \
  X(LOWEST)                                                                    \
  X(ASSIGN)                                                                    \
  X(SUM)                                                                       \
  X(PRODUCT)                                                                   \
  X(PREFIX)                                                                    \
  X(CALL)

enum class Prec : u8 {
#define X(prec) prec,
  _PREC_
#undef X
};

struct Parser {
private:
  Lexer &lexer_;
  Arena *arena_;
  LexerState state_;
  Interner &interner_;
  Diag& diag_;

public:
  Parser(Lexer &lexer, Arena &arena, Interner *interner, Diag& diag)
      : lexer_(lexer), arena_(&arena), state_{0}, interner_(*interner), diag_(diag) {}
  Node *parse_prog();

  // private:
  /* HELPERS */
  Token peek() const;
  Token consume_token();
  bool match(TokenKind kind);
  // Token expect(TokenKind kind);

  void synchronize();

  Option<u32> parse_type();
  /* recursive descent parser */
  Node *parse_decl();
  Node *parse_fn();
  Node *parse_params();

  Node *parse_block();
  Node *parse_stmt();
  Node *parse_if();
  Node *parse_while();
  Node *parse_ret();

  /* PRATT PARSER */
  Node *parse_expr(Prec prec = Prec::LOWEST);
  Node *parse_nud();                     // parses prefix
  Node *parse_led(Node *left, Token op); // parses infix and prefix

  Prec token_prec(TokenKind kind) const;
};
