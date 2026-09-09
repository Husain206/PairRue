#pragma once

#include "macros.hpp"
#include "types.hpp"
#include <type_traits>
#include <utility>

template <typename T, typename E> struct [[nodiscard]] Result {
private:
  struct Ok {
    T value;
    template <typename... Args>
    explicit Ok(Args &&...args) : value(std::forward<Args>(args)...) {}
  };
  struct Err {
    E err;
    template <typename... Args>
    explicit Err(Args &&...args) : err(std::forward<Args>(args)...) {}
  };
  variant<Ok, Err> value_;

  explicit Result(Ok val) : value_(std::move(val)) {}
  explicit Result(Err err) : value_(std::move(err)) {}
  template <typename... Args>
  explicit Result(std::in_place_index_t<0>, Args &&...args)
      : value_(std::in_place_index<0>, std::forward<Args>(args)...) {}
  template <typename... Args>
  explicit Result(std::in_place_index_t<1>, Args &&...args)
      : value_(std::in_place_index<1>, std::forward<Args>(args)...) {}

public:
  // if we enable a default constructor, we'll need to introduce std::monostate
  // as the std::variant first variant even tho its gonna be an implmentation
  // state, its gonna look more like a Result<None, OK, Err> where None is
  // defualt constructed that also means we'll have to check if is_none() or is
  // neither ok nor err before transforming so no, just delete it
  Result() = delete;
  Result(const Result &) = default;
  Result(Result &&) = default;

  Result &operator=(Result &&) = default;
  Result &operator=(const Result &) = default;

  template <typename... Args> static Result ok(Args &&...args) {
    return Result(std::in_place_index<0>, std::forward<Args>(args)...);
  }
  template <typename... Args> static Result err(Args &&...args) {
    return Result(std::in_place_index<1>, std::forward<Args>(args)...);
  }

  bool is_ok() const noexcept { return value_.index() == 0; }
  bool is_err() const noexcept { return value_.index() == 1; }

  /*
   * the suffixes mean ref qualifiers
   * &: callable on lvalue object -> x.value();
   * &&: callable on rvalue object -> std::move(x).value();
   * c++ is on drugs istg
   */
  T &value() & {
    ASSERT(is_ok());
    return std::get<Ok>(value_).value;
  }
  const T &value() const & {
    ASSERT(is_ok());
    return std::get<Ok>(value_).value;
  }
  T &&value() && {
    ASSERT(is_ok());
    return std::move(std::get<Ok>(value_).value);
  }
  const T &&value() const && {
    ASSERT(is_ok());
    return std::move(std::get<Ok>(value_).value);
  }

  E &error() & {
    ASSERT(is_err());
    return std::get<Err>(value_).err;
  }
  const E &error() const & {
    ASSERT(is_err());
    return std::get<Err>(value_).err;
  }
  E &&error() && {
    ASSERT(is_err());
    return std::move(std::get<Err>(value_).err);
  }
  const E &&error() const && {
    ASSERT(is_err());
    return std::move(std::get<Err>(value_).err);
  }

  template <typename F> auto and_then(F &&f) const -> std::invoke_result_t<F, const T&> {
    using retType = std::invoke_result_t<F, const T&>;
    if (is_err())
      return retType::err(error());
    return std::forward<F>(f)(value());
  }

  template <typename F>
  auto map(F &&f) const -> Result<std::invoke_result_t<F, const T&>, E> {
    using retType = std::invoke_result_t<F, const T&>;
    if (is_err())
      return Result<retType, E>::err(error());
    return Result<retType, E>::ok(f(value()));
  }

  template<typename F>
  auto map_err(F&& f) const -> Result<T, std::invoke_result_t<F, const E&> > {
    using retType = std::invoke_result_t<F, const E&>;
    if(is_ok())
      return Result<T, retType>::ok(value());
    return Result<T, retType>::err(std::forward<F>(f)(error()));
  }
};
