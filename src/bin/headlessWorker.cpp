//!
//! A demo using Stateline to sample from a Gaussian mixture.
//!
//! This file aims to be a tutorial on setting up a MCMC simulation using
//! the C++ worker API of Stateline.
//!
//! \file demoWorker.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \licence Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <iostream>
#include <functional>
#include <string>

#include <chrono>
#include <thread>
#include <boost/program_options.hpp>

#include "app/logging.hpp"
#include "app/commandline.hpp"
#include "comms/worker.hpp"
#include "app/signal.hpp"
#include "stats/mixture.hpp"

namespace sl = stateline;
namespace po = boost::program_options;
namespace ph = std::placeholders;
namespace ch = std::chrono;

po::options_description commandLineOptions()
{
  auto opts = po::options_description("Demo Options");
  opts.add_options()
    ("loglevel,l", po::value<int>()->default_value(0), "Logging level")
    ("address,a",po::value<std::string>()->default_value("localhost:5555"), "Address of delegator")
    ;
  return opts;
}

int main(int ac, char *av[])
{
  // --------------------------------------------------------------------------
  // Initialise command line options and logging
  // --------------------------------------------------------------------------
  po::variables_map vm = sl::parseCommandLine(ac, av, commandLineOptions());
  
  // Initialise the logging settings
  sl::initLogging("client", vm["loglevel"].as<int>(), true, "");
  
  // This allows the user to interrupt using Ctrl-C.
  sl::init::initialiseSignalHandler();


  // --------------------------------------------------------------------------
  // Initialise the worker
  // --------------------------------------------------------------------------
  std::string address = vm["address"].as<std::string>();
  sl::WorkerSettings settings = sl::WorkerSettings::Default(address);

  settings.heartbeat.msRate = 100000;
    settings.heartbeat.msTimeout = 200000;

  // In Stateline, a worker can handle multiple job types. Since the server
  // only sends out one job type, we can just set it to the default job type
  // of 0. In cases where there are more than one job type, the vector should
  // contain the job types that this worker wants to handle.
  sl::comms::Worker worker(settings);
  
  
  while(!sl::global::interruptedBySignal)
  {
    std::this_thread::sleep_for(ch::milliseconds(500));
  }

  return 0;
}
