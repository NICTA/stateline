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
    ("address,a",po::value<std::string>()->default_value("localhost:5555"), "Address of server")
    ;
  return opts;
}

double gaussianNLL(const std::string& /*jobType*/, const std::vector<double>& x)
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
  std::string address = vm["address"].as<std::string>();

  // Initialise logging
  sl::initLogging("client", logLevel);

  // Capture Ctrl+C
  sl::init::initialiseSignalHandler();
    
  // Only 1 job type ("job") for this demo
  sl::WorkerWrapper w(gaussianNLL, {"job"}, address);

  /*
   NB: For multiple likelihood functions WorkerWrapper can be initialised with a map, e.g.:
      sl::JobLikelihoodFnMap lhMap = { { "job", gaussianNLL } };
      sl::WorkerWrapper w(lhMap, address);

    or a function, e.g.:
      const sl::LikelihoodFn& lh = gaussianNLL;
      sl::JobToLikelihoodFnFn f = [&lh](const std::string&) -> const sl::LikelihoodFn& { return lh; };
      sl::WorkerWrapper w(f, {"job"}, address);
  */

  w.start();

  while(!sl::global::interruptedBySignal)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  w.stop();

  return 0;
}
