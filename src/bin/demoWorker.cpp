//!
//! Simple demo using Stateline to sample from a multidimensional Gaussian mixture.
//!
//! \file demoWorker.cpp
//! \author Darren Shen
//! \date 2014
//! \licence Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <iostream>
#include <functional>
#include <string>

#include <chrono>
#include <thread>

#include "app/logging.hpp"
#include "app/worker.hpp"
#include "stats/mixture.hpp"

namespace sl = stateline;

int main(int ac, char *av[])
{
  // Initialise logging
  sl::initLogging("client", 0, true, "");

  // Initialise workers
  sl::WorkerSettings settings = sl::WorkerSettings::Default("localhost:5555");
  
  // Create a worker to communicate with the server
  sl::comms::Worker worker({ 0 }, settings);

  // The job spec contains the parameters of the distribution that we are
  // dealing with, namely, the means of the components in the Gaussian mixture.
  auto means = sl::unserialise<Eigen::MatrixXd>(worker.globalSpec());

  // Create the distribution that we are sampling from.
  std::vector<sl::stats::Normal> components;
  for (Eigen::VectorXd::Index i = 0; i < means.rows(); i++)
    components.push_back(sl::stats::Normal(means.row(i))); // Add a component

  sl::stats::GaussianMixture distribution(components);

  // Get the PDF function of the distribution
  auto logl = [&](const Eigen::VectorXd &x)
  {
    return -sl::stats::logpdf(distribution, x);
  };

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
