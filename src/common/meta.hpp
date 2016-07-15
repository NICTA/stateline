//! Common meta-programming helpers.
//!
//! \file common/meta.hpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <cassert>
#include <tuple>
#include <initializer_list>

namespace stateline {

namespace meta {

namespace detail {

template<class T, class... Tail>
constexpr auto makeArray(T head, Tail... tail) -> std::array<T, 1 + sizeof...(Tail)>
{
  return { { head, tail... } };
}

template <class T, class F, std::size_t...Is>
void applyAllImpl(T&& t, F func, std::index_sequence<Is...>)
{
  int l[] = { (func(std::get<Is>(t)), 0)... };
  (void)l;
}

template <class T, class F, std::size_t...Is>
void enumerateAllImpl(T&& t, F func, std::index_sequence<Is...>)
{
  int l[] = { (func(Is, std::get<Is>(t)), 0)... };
  (void)l;
}

template <class T, class F, std::size_t...Is>
constexpr decltype(auto) mapAllImpl(T&& t, F func, std::index_sequence<Is...>)
{
  return makeArray(func(std::get<Is>(t))...);
}

} // namespace detail

template <class... Ts, class F>
void applyAll(const std::tuple<Ts...>& t, F func)
{
  detail::applyAllImpl(t, func, std::index_sequence_for<Ts...>{});
}

template <class... Ts, class F>
void enumerateAll(std::tuple<Ts...>& t, F func)
{
  detail::enumerateAllImpl(t, func, std::index_sequence_for<Ts...>{});
}

template <class... Ts, class F>
constexpr decltype(auto) mapAll(const std::tuple<Ts...>& t, F func)
{
  return detail::mapAllImpl(t, func, std::index_sequence_for<Ts...>{});
}

} } // namespace stateline
