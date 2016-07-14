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

template <class T>
void packValue(std::string& buf, T& val)
{
  buf.append(reinterpret_cast<const char*>(&val), sizeof(T));
}

template <class T>
void packRange(std::string& buf, const T *first, const T *last)
{
  buf.append(reinterpret_cast<const char*>(first), (last - first) * sizeof(T));
}

struct Unpacker
{
  const char *first;
  const char *last;

  explicit Unpacker(const std::string& buf)
    : first{buf.data()}, last{buf.data() + buf.size()}
  {
  }

  template <class T>
  void unpackValue(T& val)
  {
    assert(first + sizeof(T) <= last);

    val = reinterpret_cast<const T&>(*first);
    first += sizeof(T);
  }

  template <class T>
  T unpackValue()
  {
    T val;
    unpackValue(val);
    return val;
  }

  template <class Container>
  void unpackRest(Container& container)
  {
    using T = typename Container::value_type;
    assert((last - first) % sizeof(T) == 0);

    container.reserve((last - first) / sizeof(T));

    for (; first != last; first += sizeof(T))
      container.push_back(reinterpret_cast<const T&>(*first));
  }
};

} }
