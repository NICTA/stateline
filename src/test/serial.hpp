/**
 * Contains common testing utility functions
 *
 * @file common.hpp
 * @author Nahid Akbar
 * @date 2014-06-10
 * @license General Public License version 3 or later
 * @copyright (c) 2013, NICTA
 */

#pragma once

#include "common.hpp"
#include "input/input.hpp"

namespace obsidian
{
  class Serialise: public ::testing::Test
  {
  };

  template<typename T> void test(const T & original)
  {
    std::string encoded = comms::serialise(original);
    T decoded;
    comms::unserialise(encoded, decoded);
    EXPECT_EQ(decoded, original);
    std::string reencoded = comms::serialise(decoded);
    EXPECT_EQ(encoded, reencoded);
  }

}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
