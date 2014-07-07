//!
//! Settings constructs for program configuration
//!
//! \file app/settings.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

// Project
#include "comms/settings.hpp"

// Standard Library / Prerequisites
#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace obsidian
{
  void readConfigFile(const std::string& configFilename, po::variables_map& vm);
  stateline::DelegatorSettings parseDelegatorSettings(const po::variables_map& vm);
  stateline::WorkerSettings parseWorkerSettings(const po::variables_map& vm);
  stateline::DBSettings parseDBSettings(const po::variables_map& vm);
}
