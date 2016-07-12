//! Interface to the external binary protocol.
//!
//! \file src/comms/binary.hpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <array>
#include <cassert>
#include <string>

namespace stateline { namespace comms {

template <class... Args>
struct PackSize;

template <class T, class... Args>
struct PackSize<T, Args...>
{
  static constexpr std::size_t value = sizeof(T) + PackSize<Args...>::value;
};

template <>
struct PackSize<> { static constexpr std::size_t value = 0; };

namespace detail {

char* packBufferImpl(char* buf) { return buf; }

template <class T, class... Args>
char* packBufferImpl(char* buf, T val, Args... args)
{
  memcpy(buf, reinterpret_cast<char*>(&val), sizeof(val));
  return packBufferImpl(buf + sizeof(val), args...);
}

} // namespace detail

template <class... Args>
std::string packBuffer(Args... args)
{
  // Preallocate the buffer
  std::string buffer(PackSize<Args...>::value, ' ');
  detail::packBufferImpl(&buffer[0], args...);
  return buffer;
}

namespace detail {

// TODO: there's gotta be a better way of doing this
template <class... Args, class Iterator, class...Done>
typename std::enable_if<
  sizeof...(Args) == 0,
  std::pair<std::tuple<Done...>, Iterator>
>::type unpackBufferImpl(Iterator first, Iterator, Done... done)
{
  return {std::forward_as_tuple(done...), first};
}

template <class T, class... Args, class Iterator, class... Done>
std::pair<std::tuple<T, Args..., Done...>, Iterator> unpackBufferImpl(Iterator first, Iterator last, Done... done)
{
  assert(std::distance(last, first) >= sizeof(T));

  T val = reinterpret_cast<const T&>(*first);
  return unpackBufferImpl<Args...>(first + sizeof(T), last, done..., val);
}

}

template <class... Args, class Iterator>
std::pair<std::tuple<Args...>, Iterator> unpackBuffer(Iterator first, Iterator last)
{
  return detail::unpackBufferImpl<Args...>(first, last);
}

template <class... Args>
std::tuple<Args...> unpackBuffer(const std::string& str)
{
  return detail::unpackBufferImpl<Args...>(str.begin(), str.end()).first;
}

} }
