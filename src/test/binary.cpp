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

TEST_CASE("can pack empty tuple", "[binary]")
{
  REQUIRE(packBuffer() == "");
}

TEST_CASE("can unpack empty string", "[binary]")
{
  std::string str = "";
  const auto result = unpackBuffer(str.begin(), str.end());
  REQUIRE(std::tuple_size<decltype(result.first)>::value == 0);
  REQUIRE(result.second == str.end());
}

TEST_CASE("can pack and unpack one item", "[binary]")
{
  const auto result = unpackBuffer<std::int32_t>(packBuffer(std::int32_t{42}));
  REQUIRE(std::get<0>(result) == 42);
}

TEST_CASE("can pack and unpack two items", "[binary]")
{
  const auto result = unpackBuffer<std::int32_t, std::int8_t>(packBuffer(std::int32_t{42}, std::int8_t{1}));
  REQUIRE(std::get<0>(result) == 42);
  REQUIRE(std::get<1>(result) == 1);
}

