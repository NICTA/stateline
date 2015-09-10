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

#include "../app/logging.hpp"
#include "../app/commandline.hpp"
#include "../comms/worker.hpp"
#include "../app/signal.hpp"
#include "../stats/mixture.hpp"
#include "../comms/thread.hpp"

namespace sl = stateline;
namespace po = boost::program_options;
namespace ph = std::placeholders;
namespace ch = std::chrono;

po::options_description commandLineOptions()
{
  auto opts = po::options_description("Stateline client worker options");
  opts.add_options()
    ("help,h", "Print help message")
    ("loglevel,l", po::value<int>()->default_value(0), "Logging level")
    ("networkAddr,n",po::value<std::string>()->default_value("localhost:5555"), "Address of delegator")
    ("workerAddr,w",po::value<std::string>()->default_value("ipc:///tmp/sl_worker.sock"),
      "Address of worker for minion to connect to")
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
  std::string networkAddr = vm["networkAddr"].as<std::string>();
  std::string workerAddr = vm["workerAddr"].as<std::string>();
  sl::comms::WorkerSettings settings = sl::comms::WorkerSettings::Default(networkAddr, workerAddr);

  // In Stateline, a worker can handle multiple job types. Since the server
  // only sends out one job type, we can just set it to the default job type
  // of 0. In cases where there are more than one job type, the vector should
  // contain the job types that this worker wants to handle.
  zmq::context_t* context = new zmq::context_t(1);
  bool running = true;
  auto future = sl::startInThread<sl::comms::Worker>(running, std::ref(*context), std::cref(settings));

  while(!sl::global::interruptedBySignal)
  {
    std::this_thread::sleep_for(ch::milliseconds(500));
  }

  running = false;
  delete context;

  future.wait();

  return 0;
}
