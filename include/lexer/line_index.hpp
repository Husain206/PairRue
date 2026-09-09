#pragma once

#include "../core/types.hpp"
#include "span.hpp"
#include <algorithm>
#include <iterator>

struct LineIndex {
  private:
    vec<i32> line_starts_;

  public:
  explicit LineIndex(strview src){
    // line 1 always start at byte offset 0
    line_starts_.push_back(0);

    for(usize i = 0; i < src.size(); ++i){
      if(src[i] == '\n')
        line_starts_.push_back(i+1); // the char after '\n' is the line start
    }
  }

  // lazy binary search resolution
  [[nodiscard]] Location lookup(u32 offset) const noexcept {
    if(line_starts_.empty()) return {1, 1};

    // Find the first line start that is strictly greater than offset
    auto it = std::upper_bound(line_starts_.begin(), line_starts_.end(), offset);

    usize line_idx = std::distance(line_starts_.begin(), it) - 1;
    u32 line_start_offset = line_starts_[line_idx];

    u32 line = static_cast<u32>(line_idx+1);
    u32 col = static_cast<u32>(offset - line_start_offset) + 1;

    return {line, col};
  }
};
