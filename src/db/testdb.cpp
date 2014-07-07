//!
//!
//! \file db/testdb.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "db/testdb.hpp"
#include "app/console.hpp"

const int logLevel = -3;
const bool stdErr = false;
std::string directory = ".";

int main(int ac, char** av)
{
  obsidian::init::initialiseLogging("testdb", logLevel, stdErr, directory);
  testing::InitGoogleTest(&ac, av);
  auto result = RUN_ALL_TESTS();
  return result;
}

