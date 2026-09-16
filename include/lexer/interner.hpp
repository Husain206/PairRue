#pragma once

// #include "../core/hashmap.hpp"
#include "../core//macros.hpp"
#include "../core/types.hpp"
#include <unordered_map>

#define SV(interner, id) static_cast<int>(interner.view(id).length()), interner.view(id).data()

struct Interner {
private:
  // Hashmap<strview, ssize> pool_;
  std::unordered_map<strview, u32> pool_{};
  vec<string> from_id_{};

public:
  ssize intern(strview sv) {
    auto found = pool_.find(sv);
    if (found != pool_.end())
      return found->second;
    auto id = from_id_.size();
    from_id_.push_back(static_cast<string>(sv));
    pool_.emplace(sv, id);
    return id;
  };

  [[nodiscard]] strview view(ssize id) const {
    ASSERT(id < from_id_.size());
    return from_id_[id];
  }
};
