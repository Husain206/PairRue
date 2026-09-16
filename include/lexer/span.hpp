#pragma once

#include "../core/types.hpp"
#include "../core/macros.hpp"

struct Location {
  u32 line{1};
  u32 col{1};
};

struct Span {
  const char* start{nullptr};
  usize len{0};
  Location loc{};

  [[nodiscard]] constexpr strview to_strview() const noexcept {
    return {start, len};
  }
  [[nodiscard]] constexpr bool empty() const noexcept {
    return len == 0;
  }
};

inline auto span_slice(Span src, usize offset, usize length) -> Span {
  ASSERT(offset <= src.len);
  ASSERT(offset + length <= src.len);
  return { .start = src.start + offset, .len = length, .loc = src.loc };  
}
