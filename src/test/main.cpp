#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include "app/logging.hpp"

const int logLevel = -3;
const bool stdErr = false;
const std::string filename = "test.log";

int main(int argc, const char* argv[]) {
  stateline::initLogging(logLevel, stdErr, filename);
  return Catch::Session().run(argc, argv);
}
