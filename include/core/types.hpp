#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>


/* SIGNED */
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using ssize = ptrdiff_t;

/* UNSIGNED */
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using usize = size_t;

/* STRING */
using cstr = char *;
using ccstr = const char *;
using string = std::string;
using strview = std::string_view;

/* STL */
template <typename T, typename U> using pair = std::pair<T, U>;
template <typename T> using vec = std::vector<T>;
template <typename... Args> using variant = std::variant<Args...>;
