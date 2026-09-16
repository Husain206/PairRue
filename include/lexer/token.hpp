#pragma once

#include "../core/types.hpp"
#include "span.hpp"

#define _SPECIAL_ X(TK_EoF, "EOF") X(TK_INVALID, "INVALID")
#define _KEYWORDS_                                                             \
  X(TK_FN, "fn")                                                               \
  X(TK_RET, "ret") X(TK_LET, "let") X(TK_I32, "i32") X(TK_STRING, "string")    \
  X(TK_IF, "if") X(TK_ELSE, "else") X(TK_WHILE, "while")
#define _LITERALS_                                                             \
  X(TK_ID, "ID")                                                               \
  X(TK_INT_LITERAL, "INT_LITERAL") X(TK_STR_LITERAL, "STR_LITERAL")
#define _DELIM_                                                                \
  X(TK_LPRN, "(")                                                              \
  X(TK_RPRN, ")")                                                              \
  X(TK_LBRK, "[") X(TK_RBRK, "]") X(TK_LBRACE, "{") X(TK_RBRACE, "}")
#define _SYMBOLS_                                                              \
  X(TK_PLUS, "+")                                                              \
  X(TK_MINUS, "-")                                                             \
  X(TK_STAR, "*")                                                              \
  X(TK_SLASH, "/")                                                             \
  X(TK_AMP, "&")                                                               \
  X(TK_COLON, ":") X(TK_SEMICOLON, ";") X(TK_COMMA, ",") X(TK_ASSIGN, "=")

enum class TokenKind {
#define X(token, tname) token,
  _SPECIAL_ _KEYWORDS_ _LITERALS_ _DELIM_ _SYMBOLS_
#undef X
};

struct Token {
  TokenKind kind = TokenKind::TK_EoF;
  Span span{};
  union {
    u64 int_val;
    u32 symbol_id;
  };
};

inline ccstr token_kind_to_str(TokenKind kind) noexcept {
  switch (kind) {
#define X(token, tname)                                                        \
  case TokenKind::token:                                                       \
    return tname;
    _SPECIAL_ _KEYWORDS_ _LITERALS_ _DELIM_ _SYMBOLS_
#undef X
  }
  return "UNKNOWN TOKEN";
}

#define _LexErr_                                                               \
  X(INVALID_CHARACTER)                                                         \
  X(UNTERMINATED_STRING_LITERAL)                                               \
  X(UNTERMINATED_COMMENTS)                                                     \
  X(INVALID_NUMERIC_CONSTANT)

enum class ScanErrorKind {
#define X(err) err,
  _LexErr_
#undef X
};

struct ScanError {
  u32 offset;
  ScanErrorKind err_kind;
};

inline ccstr scan_err_to_str(ScanErrorKind kind) noexcept {
  switch (kind) {
#define X(err)                                                                 \
  case ScanErrorKind::err:                                                     \
    return #err;
    _LexErr_
#undef X
  }
  return "UNKNOWN ERROR";
}
