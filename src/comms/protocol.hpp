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
  std::pair<std::uint32_t, std::uint32_t> jobTypesRange;
  std::uint32_t hbTimeoutSecs;

  template <class Pack>
  void pack(Pack& p)
  {
    p.reserve(4 + 4 + 4);
    p.value(jobTypesRange.first);
    p.value(jobTypesRange.second);
    p.value(hbTimeoutSecs);
  }
};

struct Welcome
{
  std::uint32_t hbTimeoutSecs;

  template <class Pack>
  void pack(Pack& p)
  {
    p.reserve(4);
    p.value(hbTimeoutSecs);
  }
};

struct Job
{
  std::uint32_t id;
  std::uint32_t type;
  std::vector<double> data;

  template <class Pack>
  void pack(Pack& p)
  {
    p.reserve(4 + 4 + data.size() * sizeof(double));
    p.value(id);
    p.value(type);
    p.rawRange(data);
  }
};

struct Result
{
  std::uint32_t id;
  double data;

  template <class Pack>
  void pack(Pack& p)
  {
    p.reserve(4 + sizeof(double));
    p.value(id);
    p.value(data);
  }
};

struct BatchJob
{
  std::uint32_t id;
  std::vector<double> data;

  template <class Pack>
  void pack(Pack& p)
  {
    p.reserve(4 + data.size() * sizeof(double));
    p.value(id);
    p.rawRange(data);
  }
};

struct BatchResult
{
  std::uint32_t id;
  std::vector<double> data;

  template <class Pack>
  void pack(Pack& p)
  {
    p.reserve(4 + data.size() * sizeof(double));
    p.value(id);
    p.rawRange(data);
  }
};

template <class T>
std::string serialise(const T& t)
{
  std::string buf;
  Packer packer{buf};
  const_cast<T&>(t).pack(packer); // trust me
  return buf;
}

template <class T>
T unserialise(const std::string& str)
{
  T t;
  Unpacker unpacker{str};
  t.pack(unpacker);
  return t;
}

} } }
