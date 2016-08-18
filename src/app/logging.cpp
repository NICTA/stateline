//!
//! Contains the implementation of logging.
//!
//! \file app/logging.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "app/logging.hpp"

#include <vector>
#include <cstdlib>
#include <iostream>

#define ELPP_DISABLE_DEFAULT_CRASH_HANDLING
#include <easylogging/easylogging++.h>

// Don't want to get annoying crash messages when we ctrl-c
INITIALIZE_EASYLOGGINGPP

namespace stateline {

namespace {

int logLevelToInt(const std::string& logLevel)
{
  if (logLevel == "INFO")
    return 1;
  if (logLevel == "DEBUG")
    return 2;
  if (logLevel == "TRACE")
    return 3;
  else
    return 0;
}

}

void initLogging(const std::string& logLevel, bool stdOut, const std::string& filename)
{
  el::Configurations defaultConf;
  defaultConf.setToDefault();

  defaultConf.setGlobally(
      el::ConfigurationType::Format, "%datetime [%level] %fbase:%line: %msg");
  defaultConf.setGlobally(
      el::ConfigurationType::ToStandardOutput, stdOut ? "true" : "false");
  defaultConf.setGlobally(
      el::ConfigurationType::ToFile, stdOut ? "false" : "true");
  defaultConf.setGlobally(
      el::ConfigurationType::Filename, filename);
  defaultConf.setGlobally(
      el::ConfigurationType::Enabled, "false"); // Disable all logs first

  defaultConf.set(el::Level::Info,
      el::ConfigurationType::Format, "%datetime [%level] %msg"); // No need for file names on INFO logs

  defaultConf.set(el::Level::Error,
      el::ConfigurationType::Enabled, "true");
  defaultConf.set(el::Level::Warning,
      el::ConfigurationType::Enabled, "true");

  const auto logLevelInt = logLevelToInt(logLevel);

  if (logLevelInt >= 1)
    defaultConf.set(el::Level::Info,
        el::ConfigurationType::Enabled, "true");

  if (logLevelInt >= 2)
    defaultConf.set(el::Level::Debug,
        el::ConfigurationType::Enabled, "true");

  if (logLevelInt >= 3)
    defaultConf.set(el::Level::Trace,
        el::ConfigurationType::Enabled, "true");

  el::Loggers::reconfigureLogger("default", defaultConf);

  LOG(INFO) << "Logging initialised with level " << logLevel;
}

} // namespace stateline
