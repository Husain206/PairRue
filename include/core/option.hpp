#pragma once

#include "types.hpp"
#include <type_traits>
#include <utility>
#include "macros.hpp"


struct NoneType{};
inline constexpr NoneType None{};

template <typename T> struct Option {
private:
  variant<NoneType, T> value_;

  // maybe not
  // template<typename... Args>
  // explicit Option(Args&&... args) : value_(std::forward<Args>(args)...) {}

public:
  Option() : value_(None) {}
  explicit Option(NoneType) : value_(None) {}

  Option(const Option&) = default;
  Option(Option&&) = default;

  Option& operator=(Option&&) = default;
  Option& operator=(const Option&) = default;

  template<typename... Args>
  static Option some(Args&&... args) {
    Option result;
    result.value_.template emplace<T>(std::forward<Args>(args)...);
    return result;
  }

  static Option none() {
    return Option{};
  }

  bool is_some() const noexcept { return value_.index() == 1; }
  bool is_none() const noexcept { return value_.index() == 0; }

  T& value() & { ASSERT(is_some()); return std::get<T>(value_); }
  const T& value() const & { ASSERT(is_some()); return std::get<T>(value_); }
  T&& value() && { ASSERT(is_some()); return std::move(std::get<T>(value_)); }
  const T&& value() const && { ASSERT(is_some()); return std::move(std::get<T>(value_)); }

  T value_or(T fallback) const { return is_some() ? std::get<T>(value_) : fallback; }
  T value_or(const T& fallback) const & { return is_some() ? std::get<T>(value_) : fallback; }
  T value_or(T&& fallback) const && { return is_some() ? std::move(std::get<T>(value_)) : std::move(fallback); }

  template<typename F>
  auto and_then(F&& f) const -> std::invoke_result_t<F, const T&> {
    using retType = std::invoke_result_t<F, const T&>;
    if(is_none())
      return retType::none();
    return std::forward<F>(f)(value());
  }

  template<typename F>
  auto map(F&& f) const -> Option<std::invoke_result_t<F, const T&>> {
    using retType = Option<std::invoke_result_t<F, const T&>>;
    if(is_none())
      return Option<retType>::none();
    return Option<retType>::some(f(value()));
  }
};
