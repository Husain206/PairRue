#pragma once
#include "../core/types.hpp"
#include "../core/result.hpp"
#include "interner.hpp"
#include "line_index.hpp"
#include "span.hpp"
#include "token.hpp"
#include "../diagnostic/diag.hpp"

struct LexerState {
  u64 cursor{0};
};

struct TokenStep {
  Token token;
  LexerState next_state;
};

struct Lexer {
  private:
    strview src_;
    LineIndex line_index_{src_};
    Interner& interner_;

    char peek(LexerState state) const noexcept {
      if(state.cursor >= src_.size()) return '\0';
      return src_[state.cursor];
    }
    char peek_next(LexerState state) const noexcept {
      if(state.cursor + 1 >= src_.size()) return '\0';
      return src_[state.cursor + 1];
    }

    LexerState skip_whitespace_and_comments(LexerState state) const noexcept;
    TokenKind match_keyword(strview ident) const noexcept;

    Result<TokenStep, ScanError> string(LexerState state, u32 start_offset) noexcept;
    Result<TokenStep, ScanError> number(LexerState state, u32 start_offste) const noexcept;
    Result<TokenStep, ScanError> single_char(LexerState state, u32 start_offste) const;
    Result<TokenStep, ScanError> double_char(LexerState state, u32 start_offste) const;


  public:
    explicit Lexer(strview src, Interner& interner, Diag& diag) : src_(src), interner_(interner), diag(diag) {}

    Diag& diag;

    [[nodiscard]] Result<TokenStep, ScanError> next_token(LexerState state) noexcept;
    [[nodiscard]] Location resolve_location(u32 offset) const noexcept {
      return line_index_.lookup(offset);
    }

    [[nodiscard]] strview src() const noexcept { return src_; }
};
