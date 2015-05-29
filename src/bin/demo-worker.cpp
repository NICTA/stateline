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

#include <string>
#include <thread>
#include <boost/program_options.hpp>

#include "../app/commandline.hpp"
#include "../app/signal.hpp"
#include "../app/logging.hpp"
#include "../app/workerwrapper.hpp"

namespace sl = stateline;
namespace po = boost::program_options;

po::options_description commandLineOptions()
{
  auto opts = po::options_description("Demo Options");
  opts.add_options()
    ("loglevel,l", po::value<int>()->default_value(0), "Logging level")
    ("nthreads,t", po::value<uint>()->default_value(1), "Number of worker threads")
    ("address,a",po::value<std::string>()->default_value("localhost:5555"), "Address of server")
    ;
  return opts;
}

double gaussianNLL(const std::string& jobType, const std::vector<double>& x)
{
  double squaredNorm = 0.0;
  for (auto i : x)
  {
    squaredNorm += i*i; 
  }
  return 0.5*squaredNorm;
}


int main(int ac, char *av[])
{

  // Parse the command line
  po::variables_map vm = sl::parseCommandLine(ac, av, commandLineOptions());
  int logLevel = vm["loglevel"].as<int>();
  uint nThreads = vm["nthreads"].as<uint>();
  std::string address = vm["address"].as<std::string>();

  // Initialise logging
  sl::initLogging("client", logLevel);
  // Capture Ctrl+C
  sl::init::initialiseSignalHandler();
    
  // Only 1 job type for this demo
  std::vector<std::string> jobTypes {"job"};

  sl::WorkerWrapper w(gaussianNLL, address, jobTypes, nThreads);
  w.start();

  while(!sl::global::interruptedBySignal)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  w.stop();

  return 0;
}
