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
#include "ezoptionparser/ezOptionParser.hpp"

#include "../app/logging.hpp"
#include "../app/commandline.hpp"
#include "../comms/worker.hpp"
#include "../app/signal.hpp"
#include "../comms/thread.hpp"

namespace sl = stateline;
namespace ph = std::placeholders;
namespace ch = std::chrono;

ez::ezOptionParser commandLineOptions()
{
  ez::ezOptionParser opt;
  opt.overview = "Stateline client worker options";
  opt.add("", 0, 0, 0, "Print help message", "-h", "--help");
  opt.add("0", 0, 1, 0, "Logging level", "-l", "--log-level");
  opt.add("localhost:5555", 0, 1, 0, "Address of delegator", "-n", "--network-addr");
  opt.add("ipc:///tmp/sl_worker.sock", 0, 1, 0, "Address of worker for minion to connect to", "-w", "--worker-addr");
  return opt;
}

int main(int argc, const char *argv[])
{
  // Parse the command line
  auto opt = commandLineOptions();
  opt.parse(argc, argv);

  // Initialise logging
  int logLevel;
  opt.get("-l")->getInt(logLevel);
  sl::initLogging(logLevel);

  // Capture Ctrl+C
  sl::init::initialiseSignalHandler();

  // --------------------------------------------------------------------------
  // Initialise the worker
  // --------------------------------------------------------------------------
  std::string networkAddr, workerAddr;
  opt.get("-n")->getString(networkAddr);
  opt.get("-w")->getString(workerAddr);
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
