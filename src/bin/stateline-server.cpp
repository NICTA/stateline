//!
//! A demo using Stateline to sample from a Gaussian mixture.
//!
//! This file aims to be a tutorial on setting up a MCMC simulation using
//! the C++ server API of Stateline.
//!
//! \file demoDelegator.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \licence Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <iostream>
#include <functional>
#include <fstream>
#include <boost/program_options.hpp>

#include <chrono>

#include "../app/logging.hpp"
#include "../app/serial.hpp"
#include "../app/signal.hpp"
#include "../app/commandline.hpp"
#include "../comms/delegator.hpp"
#include "../comms/thread.hpp"

// Alias namespaces for conciseness
namespace sl = stateline;
namespace ph = std::placeholders;
namespace po = boost::program_options;
namespace ch = std::chrono;

po::options_description commandLineOptions()
{
  auto opts = po::options_description("Demo Options");
  opts.add_options()
  ("help,h", "Print help message")
  ("loglevel,l", po::value<int>()->default_value(0), "Logging level")
  ("port,p",po::value<uint>()->default_value(5555), "Port on which to accept worker connections") 
  ;
  return opts;
}

int main(int ac, char *av[])
{
  // --------------------------------------------------------------------------
  // Settings for the demo
  // --------------------------------------------------------------------------

  po::variables_map vm = sl::parseCommandLine(ac, av, commandLineOptions());

  // This allows the user to interrupt the MCMC using Ctrl-C.
  sl::init::initialiseSignalHandler();

  // Initialise the logging settings
  sl::initLogging("server", vm["loglevel"].as<int>(), true, "");

  uint port = vm["port"].as<uint>();
  auto settings = sl::comms::DelegatorSettings::Default(port);
  settings.heartbeat.msRate = 100000;
  settings.heartbeat.msTimeout = 200000;
  zmq::context_t* context = new zmq::context_t(1);
  LOG(INFO) << "\033[1;31mstarting delegator in thread\033[0m";
  bool running = true;
  auto future = sl::startInThread<sl::comms::Delegator>(running, std::ref(*context), std::cref(settings));
  //sl::comms::Delegator delegator(context, settings);
  //delegator.start();
  LOG(INFO) << "started delegator in thread";

  while(!sl::global::interruptedBySignal)
  {
    std::this_thread::sleep_for(ch::milliseconds(500));
  }

  running = false;
  delete context;

  future.wait();

  return 0;
}
