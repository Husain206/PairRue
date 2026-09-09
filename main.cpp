#include "./include/core/arena.hpp"
#include "./include/core/option.hpp"
#include "./include/core/result.hpp"
#include "include/core/types.hpp"
#include <cstring>
#include <iostream>
#include <optional>
#include <variant>

using i32 = int;

struct Node {
  const char *name;
  i32 id;
};

enum Error {
  UNKNOWN_REG,
};

Result<ccstr, Error> build_temporary_reg(Arena &scratch, const char *prefix,
                                         int id) {
  if (strncmp(prefix, "reg", 3) != 0)
    return Result<ccstr, Error>::err(UNKNOWN_REG);
  char *buf = static_cast<char *>(scratch.alloc(64, 1));
  snprintf(buf, 64, "__temp_%s_%d", prefix, id);

  return Result<ccstr, Error>::ok(buf);
}

auto main(void) -> i32 {

  Arena permanent_arena(KiB(1));
  auto *node1 =
      static_cast<Node *>(permanent_arena.alloc(sizeof(Node), alignof(Node)));
  node1->name = "GlobalVar";
  node1->id = 1;
  std::printf("permanent memory used (before scratch): %zu bytes\n",
              permanent_arena.get_marker().used);
  {
    ScratchArena scratch(permanent_arena);

    auto temp_id1 = build_temporary_reg(*scratch.get(), "reg", 101);
    auto temp_id2 = build_temporary_reg(*scratch.get(), "reg", 102);

    std::printf("inside scratch scope:\n");
    std::printf("  temp identifiers: %s, %s\n", temp_id1.value(),
                temp_id2.value());
    std::printf("  arena memory used (in scratch): %zu bytes\n",
                permanent_arena.get_marker().used);

  }

  std::printf("permanent memory used (after scratch scope exit): %zu bytes\n",
              permanent_arena.get_marker().used);

  auto *node2 =
      static_cast<Node *>(permanent_arena.alloc(sizeof(Node), alignof(Node)));
  node2->name = "LocalVar";
  node2->id = 2;

  std::printf("final permanent memory ysed: %zu bytes\n",
              permanent_arena.get_marker().used);

  return 0;
}
