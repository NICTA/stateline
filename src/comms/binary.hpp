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

class Packer
{
public:
  explicit Packer(std::string& buf)
    : buf{buf}
  {
  }

  void reserve(std::size_t capacity)
  {
    buf.reserve(capacity);
  }

  template <class T>
  void value(const T& val)
  {
    assert(buf.size() + sizeof(T) <= buf.capacity());
    buf.append(reinterpret_cast<const char*>(&val), sizeof(T));
  }

  template <class Container>
  void rawRange(Container& container)
  {
    using T = typename Container::value_type;
    const auto first = container.data();
    const auto last = container.data() + container.size();
    assert(buf.size() + (last - first) * sizeof(T) <= buf.capacity());
    buf.append(reinterpret_cast<const char*>(first), (last - first) * sizeof(T));
  }

private:
  std::string& buf;
};

class Unpacker
{
public:
  explicit Unpacker(const std::string& buf)
    : first{buf.data()}, last{buf.data() + buf.size()}
  {
  }

  void reserve(std::size_t capacity)
  {
    // Does nothing
  }

  template <class T>
  void value(T& val)
  {
    assert(first + sizeof(T) <= last);

    val = reinterpret_cast<const T&>(*first);
    first += sizeof(T);
  }

  template <class Container>
  void rawRange(Container& container)
  {
    using T = typename Container::value_type;
    assert((last - first) % sizeof(T) == 0);

    container.reserve((last - first) / sizeof(T));

    for (; first != last; first += sizeof(T))
      container.push_back(reinterpret_cast<const T&>(*first));
  }

private:
  const char *first;
  const char *last;
};

} }
