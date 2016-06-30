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

template<int... Is> struct seq {};
template<int N, int... Is> struct gen_seq : gen_seq<N-1, N-1, Is...> {};
template<int... Is> struct gen_seq<0, Is...> : seq<Is...> {};

// Inspired by http://stackoverflow.com/questions/21062864/optimal-way-to-access-stdtuple-element-in-runtime-by-index

namespace detail
{
  template <class Ret, int N, class T, class F>
  Ret apply_one(T& p, F func) { return func(std::get<N>(p)); }

  template <class Ret, class T, class F, int... Is>
  Ret apply(T& p, int index, F func, seq<Is...>)
  {
      using FT = Ret(T&, F);
      static constexpr FT* ftable[] = { &apply_one<Ret, Is, T, F>... };

      assert(index < sizeof...(Is));
      return ftable[index](p, func);
  }
}

template <class Ret, class T, class F>
Ret apply(T& p, std::size_t index, F func)
{
  return detail::apply<Ret>(p, index, func, gen_seq<std::tuple_size<T>::value>{});
}

// From http://stackoverflow.com/questions/16387354/template-tuple-calling-a-function-on-each-element
namespace detail
{

template <class T, class F, int...Is>
void apply_all_impl(T&& t, F func, seq<Is...>)
{
  std::initializer_list<int> l = { (func(std::get<Is>(t)), 0)... };
  (void)l;
}

}

template <class... Ts, class F>
void apply_all(const std::tuple<Ts...>& t, F func)
{
  detail::apply_all_impl(t, func, gen_seq<sizeof...(Ts)>());
}

} // namespace meta

} // namespace stateline
