#include "gtest/gtest.h"
#include "app/logging.hpp"

const int logLevel = -3;
const bool stdErr = false;
const std::string filename = "test.log";

int main (int ac, char** av) {
  stateline::initLogging(logLevel, stdErr, filename);
  testing::InitGoogleTest(&ac, av);
  return RUN_ALL_TESTS();
}
