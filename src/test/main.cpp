#include "gtest/gtest.h"
#include "app/logging.hpp"

const int logLevel = -3;
const bool stdErr = false;
std::string directory = ".";

int main (int ac, char** av) {
  stateline::initLogging("test", logLevel, stdErr, directory);
  testing::InitGoogleTest(&ac, av);
  return RUN_ALL_TESTS();
}
