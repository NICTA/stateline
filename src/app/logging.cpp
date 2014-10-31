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

#include <glog/logging.h>

namespace stateline
{
  void initLogging(const std::string &appName, int logLevel, bool stdErr,
      const std::string &directory)
  {
    int normLog = logLevel;
    int vLevel = 0;
    if (logLevel < 0)
    {
      vLevel = -logLevel;
      normLog = 0;
    }

    google::InitGoogleLogging(appName.c_str());
    FLAGS_logtostderr = stdErr;
    FLAGS_log_dir = directory;
    FLAGS_minloglevel = normLog;
    FLAGS_v = vLevel;
    LOG(INFO) << "Logging initialised with level " << normLog << " and verbosity " << vLevel;
  }

} // namespace stateline
