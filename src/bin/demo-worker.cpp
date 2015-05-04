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
#include <boost/algorithm/string.hpp>

#include "app/logging.hpp"
#include "app/commandline.hpp"
#include "stats/normal.hpp"
#include "comms/minion.hpp"

namespace sl = stateline;
namespace po = boost::program_options;
namespace ph = std::placeholders;

po::options_description commandLineOptions()
{
  auto opts = po::options_description("Demo Options");
  opts.add_options()
    ("address,a",po::value<std::string>()->default_value("localhost:5555"), "Address of delegator")
    ;
  return opts;
}

int main(int ac, char *av[])
{
  // --------------------------------------------------------------------------
  // Initialise command line options and logging
  // --------------------------------------------------------------------------
  ;//po::variables_map vm = sl::parseCommandLine(ac, av, commandLineOptions());
  sl::initLogging("client", 0, true, "");

  // --------------------------------------------------------------------------
  // Initialise the minion
  // --------------------------------------------------------------------------
  zmq::context_t ctx;
  sl::comms::Minion minion(ctx, { "job" });

  // This is the target distribution
  
  // Create the log likelihood function of the distribution.
  auto nll = [&](const Eigen::VectorXd& x)
  {
    return x.squaredNorm();
  };

  // --------------------------------------------------------------------------
  // Launch minion to perform work
  // --------------------------------------------------------------------------
  while (true)
  {
    std::cout << "GETTING JOB..." << std::endl;
    auto job = minion.nextJob();
    std::cout << "GOT JOB: " << job.first << "; " << job.second << std::endl;
    auto jobType = job.first;
    auto sample = job.second;

    // Compute the nll
    minion.submitResult(nll(sample));
  }

  return 0;
}
