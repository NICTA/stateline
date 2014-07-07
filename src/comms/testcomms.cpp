//!
//! \file comms/testcomms.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!
#include "comms/testrouter.hpp"
#include "comms/testdelegator.hpp"
#include "comms/testserialise.hpp"
#include "comms/testtransport.hpp"
#include "app/console.hpp"

const int logLevel = -3;
const bool stdErr = false;
std::string directory = ".";

int main(int ac, char** av)
{
  obsidian::init::initialiseLogging("testcomms", logLevel, stdErr, directory);
  testing::InitGoogleTest(&ac, av);
  auto result = RUN_ALL_TESTS();
  return result;
}

