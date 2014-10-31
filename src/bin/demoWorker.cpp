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
#include "app/worker.hpp"
#include "stats/mixture.hpp"

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
  po::variables_map vm = sl::parseCommandLine(ac, av, commandLineOptions());
  sl::initLogging("client", 0, true, "");

  // --------------------------------------------------------------------------
  // Initialise the worker
  // --------------------------------------------------------------------------
  std::string address = vm["address"].as<std::string>();
  sl::WorkerSettings settings = sl::WorkerSettings::Default(address);

  // In Stateline, a worker can handle multiple job types. Since the server
  // only sends out one job type, we can just set it to the default job type
  // of 0. In cases where there are more than one job type, the vector should
  // contain the job types that this worker wants to handle.
  std::vector<uint> jobTypes = { 0 };
  sl::comms::Worker worker(jobTypes, settings);

  // --------------------------------------------------------------------------
  // Initialise the distribution to sample from
  // --------------------------------------------------------------------------

  // The global spec contains the parameters of the distribution that we are
  // dealing with, namely, the means of the components in the Gaussian mixture.
  Eigen::MatrixXd means = sl::unserialise<Eigen::MatrixXd>(worker.globalSpec());

  // Create the distribution that we are sampling from. Stateline offers APIs
  // to some common distributions.
  std::vector<sl::stats::Normal> components;
  for (Eigen::VectorXd::Index i = 0; i < means.rows(); i++)
    components.push_back(sl::stats::Normal(means.row(i))); // Add a component

  // This is the target distribution
  sl::stats::GaussianMixture distribution(components);

  // Create the negative log likelihood function of the distribution.
  auto nll = std::bind(sl::stats::nlogpdf<sl::stats::GaussianMixture>,
      distribution, ph::_1);

  // --------------------------------------------------------------------------
  // Launch minions to perform work
  // --------------------------------------------------------------------------
  const int nthreads = 2;

  std::vector<std::future<void>> threads;
  for (int i = 0; i < nthreads; i++)
  {
    threads.push_back(std::move(sl::runMinionThreaded(worker, nll)));
  }

  // Wait for all the threads to finish
  for (int i = 0; i < nthreads; i++)
  {
    threads[i].wait();
  }

  return 0;
}
