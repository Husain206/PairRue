#include "../../include/lexer/lexer.hpp"
#include <cctype>
#include <cstdlib>

auto Lexer::skip_whitespace_and_comments(LexerState state) const noexcept
    -> LexerState {
  while (state.cursor < src_.size()) {
    char c = peek(state);
    if (isspace(c))
      state.cursor++;
    else if (c == '/' && peek_next(state) == '/') {
      while (state.cursor < src_.size() && peek(state) != '\n') {
        state.cursor++;
      }
    } else {
      break;
    }
  }

  return state;
}

auto Lexer::match_keyword(strview ident) const noexcept -> TokenKind {
#define X(token, kw)                                                           \
  if (ident == kw)                                                             \
    return TokenKind::token;
  _KEYWORDS_
#undef X
  return TokenKind::TK_ID;
}

static TokenKind is_single_char_token(char c) {
  switch (c) {
#define X(token, tname)                                                        \
  case tname[0]:                                                               \
    return TokenKind::token;
    _DELIM_ _SYMBOLS_
#undef X
        default : return TokenKind::TK_INVALID;
  }
}

auto is_double_char_token(char c1, char c2) -> TokenKind {
  switch (c1) {
#define X(token, tname)                                                        \
  case tname[0]:                                                               \
    if (tname[1] == c2)                                                        \
      return TokenKind::token;                                                 \
    else                                                                       \
      return TokenKind::TK_INVALID;
    _DELIM_ _SYMBOLS_
#undef X
        default : return TokenKind::TK_INVALID;
  }
}

auto Lexer::string(LexerState state, u32 start_offset) noexcept
    -> Result<TokenStep, ScanError> {
  u64 str_start = state.cursor;
  while (str_start < src_.size() && peek(state) != '\"') {
    if (peek(state) == '\n') {
      return Result<TokenStep, ScanError>::err(ScanError{
          (u32)start_offset, ScanErrorKind::UNTERMINATED_STRING_LITERAL});
    }
    state.cursor++;
  }

  if (state.cursor >= src_.size()) {
    return Result<TokenStep, ScanError>::err(ScanError{
        (u32)start_offset, ScanErrorKind::UNTERMINATED_STRING_LITERAL});
  }

  strview raw_str{src_.data() + str_start, state.cursor - str_start};
  state.cursor++; // consuming '"'
  u32 str_id = interner_.intern(raw_str);
  Span span{src_.data() + start_offset,
            static_cast<usize>(state.cursor - start_offset),
            resolve_location(start_offset)};

  Token tok{TokenKind::TK_STR_LITERAL, span, {.symbol_id = str_id}};
  return Result<TokenStep, ScanError>::ok(TokenStep{tok, state});
}

auto Lexer::number(LexerState state, u32 start_offste) const noexcept
    -> Result<TokenStep, ScanError> {
  while (state.cursor < src_.size() && isdigit(peek(state)))
    state.cursor++;

  strview num_str{src_.data() + start_offste, state.cursor - start_offste};
  u64 val = std::strtoull(num_str.data(), nullptr, 10);

  Span span{src_.data() + start_offste,
            static_cast<usize>(state.cursor - start_offste),
            resolve_location(start_offste)};
  Token tok{TokenKind::TK_INT_LITERAL, span, {.int_val = val}};
  return Result<TokenStep, ScanError>::ok(TokenStep{tok, state});
}

auto Lexer::single_char(LexerState state, u32 start_offset) const noexcept
    -> Result<TokenStep, ScanError> {
  switch (src_[state.cursor]) {
#define X(token, tname)                                                        \
  case tname[0]:                                                               \
    return Result<TokenStep, ScanError>::ok(                                   \
        TokenStep{Token{TokenKind::token,                                      \
                        Span{src_.data() + start_offset, 1,                    \
                             resolve_location(start_offset)},                  \
                        {}},                                                   \
                  state.cursor + 1});
    _DELIM_ _SYMBOLS_
#undef X
  }
  return Result<TokenStep, ScanError>::err(
      ScanError{start_offset, ScanErrorKind::INVALID_CHARACTER});
}

// auto Lexer::double_char(LexerState state, u32 start_offset) const noexcept
//     -> Result<TokenStep, ScanError> {
//   switch (src_[state.cursor - 1]) {
// #define X(token, tname) \
//   return Result<TokenStep, ScanError>::ok(TokenStep{ \
//       Token{TokenKind::token, Span{src_.data() + start_offset, 1, {}}, {}}, \
//       state});
//     _DELIM_ _SYMBOLS_
// #undef X
//   }
//   return Result<TokenStep, ScanError>::err(
//       ScanError{start_offset, ScanErrorKind::INVALID_CHARACTER});
// }

auto Lexer::next_token(LexerState state) noexcept
    -> Result<TokenStep, ScanError> {
  state = skip_whitespace_and_comments(state);

  u32 start_offset = static_cast<u32>(state.cursor);

  if (state.cursor >= src_.size()) {
    Span eof_span{src_.data() + start_offset, 0, {}};
    Token eof_tok{TokenKind::TK_EoF, eof_span, {.int_val = 0}};
    return Result<TokenStep, ScanError>::ok(TokenStep{eof_tok, state});
  }

  char c = src_[state.cursor];
  LexerState next_state = {state.cursor + 1};

  if (c == '\"')
    return string(next_state, start_offset);

  if (isdigit(c))
    return number(next_state, start_offset);

  if (isalpha(c)) {
    while (state.cursor < src_.size() &&
           (std::isalnum(peek(state)) || peek(state) == '_')) {
      state.cursor++;
    }
    strview ident{src_.data() + start_offset, state.cursor - start_offset};
    TokenKind kind = match_keyword(ident);
    Span span{src_.data() + start_offset,
              static_cast<usize>(state.cursor - start_offset),
              resolve_location(start_offset)};

    Token tok{kind, span, {}};
    // if (kind == TokenKind::TK_ID)
    tok.symbol_id = interner_.intern(ident);
    return Result<TokenStep, ScanError>::ok(TokenStep{tok, state});
  }

  // if (is_double_char_token(c, src_[next_state.cursor]) !=
  // TokenKind::TK_INVALID)
  //   return double_char(next_state, start_offset);
  if (is_single_char_token(c) != TokenKind::TK_INVALID)
    return single_char(state, start_offset);

  return Result<TokenStep, ScanError>::err(
      ScanError{start_offset, ScanErrorKind::INVALID_CHARACTER});
}
