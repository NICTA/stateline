//! Contains protocol messages.
//!
//! \file comms/protocol.hpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "comms/binary.hpp"

namespace stateline { namespace comms { namespace protocol {

struct Hello
{
  std::uint32_t hbTimeoutSecs;

  std::string serialise() const
  {
    std::string buffer;
    buffer.reserve(4);

    packValue(buffer, hbTimeoutSecs);
    return buffer;
  }

  void unserialise(const std::string& str)
  {
    Unpacker p{str};
    p.unpackValue(hbTimeoutSecs);
  }
};

struct Welcome
{
  std::uint32_t hbTimeoutSecs;

  std::string serialise() const
  {
    std::string buffer;
    buffer.reserve(4);

    packValue(buffer, hbTimeoutSecs);
    return buffer;
  }

  void unserialise(const std::string& str)
  {
    Unpacker p{str};
    p.unpackValue(hbTimeoutSecs);
  }
};

struct Job
{
  std::uint32_t id;
  std::uint32_t type;
  std::vector<float> data;

  std::string serialise() const
  {
    std::string buffer;
    buffer.reserve(4 + 4 + data.size() * 4);

    packValue(buffer, id);
    packValue(buffer, type);
    packRange(buffer, data.data(), data.data() + data.size());

    return buffer;
  }

  void unserialise(const std::string& str)
  {
    Unpacker p{str};
    p.unpackValue(id);
    p.unpackValue(type);
    p.unpackRest(data);
  }
};

struct Result
{
  std::uint32_t id;
  float data;

  std::string serialise() const
  {
    std::string buffer;
    buffer.reserve(4 + 4);

    packValue(buffer, id);
    packValue(buffer, data);

    return buffer;
  }

  void unserialise(const std::string& str)
  {
    Unpacker p{str};
    p.unpackValue(id);
    p.unpackValue(data);
  }
};

template <class T>
std::string serialise(const T& t)
{
  return t.serialise();
}

template <class T>
T unserialise(const std::string& str)
{
  T t;
  t.unserialise(str);
  return t;
}

} } }
