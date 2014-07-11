//!
//! Simple demo using Stateline to sample from a multidimensional Gaussian mixture.
//!
//! \file demoSimpleMcmc.cpp
//! \author Darren Shen
//! \date 2014
//! \licence Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <iostream>
#include <functional>
#include <string>
#include <boost/program_options.hpp>

#include <chrono>
#include <thread>

#include "app/logging.hpp"
#include "app/worker.hpp"
#include "stats/diagnormal.hpp"

namespace sl = stateline;

int main(int ac, char *av[])
{
  // Initialise logging
  sl::initLogging("client", -3, true, "");

  // Initialise workers
  sl::WorkerSettings settings = sl::WorkerSettings::Default("localhost:5555");
  
  // Create a worker to communicate with the server
  sl::comms::Worker worker({ 0 }, settings);

  // The job spec contains the parameters of the distribution that we are dealing with,
  // namely, the diagonal of the covariance matrix.
  auto var = sl::unserialise<Eigen::VectorXd>(worker.globalSpec());
  sl::stats::DiagNormal norm(Eigen::VectorXd::Zero(var.size()), var);

  // Get the PDF function of the distribution
  auto logl = std::bind(sl::stats::logpdf<sl::stats::DiagNormal>, norm, std::placeholders::_1);

  // Launch a minion to do work
  std::vector<std::future<void>> threads;
  for (int i = 0; i < 1; i++)
  {
    threads.push_back(std::move(sl::runMinionThreaded(worker, logl)));
  }

  // Wait for all the threads to finish
  for (int i = 0; i < 1; i++)
  {
    threads[i].wait();
  }

  return 0;
}
