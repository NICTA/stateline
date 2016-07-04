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
#include "ezoptionparser/ezOptionParser.hpp"

#include "../app/commandline.hpp"
#include "../app/signal.hpp"
#include "../app/logging.hpp"
#include "../app/workerwrapper.hpp"

namespace sl = stateline;

ez::ezOptionParser commandLineOptions()
{
  ez::ezOptionParser opt;
  opt.overview = "Demo options";
  opt.add("", 0, 0, 0, "Print help message", "-h", "--help");
  opt.add("0", 0, 1, 0, "Logging level", "-l", "--log-level");
  opt.add("localhost:5555", 0, 1, 0, "Address of server", "-a", "--address");
  opt.add("3", 0, 1, 0, "Number of job types", "-j", "--job-types");
  return opt;
}

double gaussianNLL(uint /*jobType*/, const std::vector<double>& x)
{
  double squaredNorm = 0.0;
  for (auto i : x)
  {
    squaredNorm += i*i;
  }
  return 0.5*squaredNorm;
}


int main(int argc, const char *argv[])
{
  // Parse the command line
  auto opt = commandLineOptions();
  if (!sl::parseCommandLine(opt, argc, argv))
    return 0;

  // Initialise logging
  int logLevel;
  opt.get("-l")->getInt(logLevel);
  sl::initLogging(logLevel);

  // Capture Ctrl+C
  sl::init::initialiseSignalHandler();

  // Initialise the worker wrapper
  std::string address;
  opt.get("-a")->getString(address);

  int numJobTypes;
  opt.get("-j")->getInt(numJobTypes);

  sl::WorkerWrapper w(gaussianNLL, {0, numJobTypes}, address);

  /*
   NB: For multiple likelihood functions WorkerWrapper can be initialised with an array, e.g.:
      sl::JobLikelihoodFnMap lhMap = { gaussianNLL };
      sl::WorkerWrapper w(lhMap, 0, address);

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
