//! Tests for binary packing and unpacking.
//!
//! \file comms/tests/binary.cpp
//! \author Darren Shen
//! \date 2016
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "catch/catch.hpp"

#include "comms/binary.hpp"

using namespace stateline::comms;

TEST_CASE("can pack and unpack one value", "[binary]")
{
  std::int32_t a = 42;

  std::string buf;
  packValue(buf, a);

  Unpacker p{buf};
  const auto aa = p.unpackValue<std::int32_t>();

  REQUIRE(aa == 42);
}

TEST_CASE("can pack and unpack two values", "[binary]")
{
  std::int32_t a = 42;
  std::int8_t b = 1;

  std::string buf;
  packValue(buf, a);
  packValue(buf, b);

  Unpacker p{buf};
  const auto aa = p.unpackValue<std::int32_t>();
  const auto bb = p.unpackValue<std::int8_t>();

  REQUIRE(aa == 42);
  REQUIRE(bb == 1);
}
