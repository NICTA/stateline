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
namespace ph = std::placeholders;

Eigen::VectorXd stringToVector(const std::string &str)
{
  std::cout << "received: " << str << std::endl;
  std::stringstream ss(str);

  // Read in the size of the vector
  Eigen::VectorXd::Index size;
  ss >> size;

  Eigen::VectorXd result(size);
  for (Eigen::VectorXd::Index i = 0; i < size; i++)
  {
    ss >> result(i);
  }
  return result;
}

std::string workerFunc(const sl::stats::DiagNormal &d, uint type, const std::string &globalData, const std::string &jobData)
{
  Eigen::VectorXd state = stringToVector(jobData);
  return std::to_string(sl::stats::pdf(d, state));
}

int main(int ac, char *av[])
{
  // Initialise logging
  sl::initLogging("client", -3, true, "");

  // Initialise workers
  sl::WorkerSettings settings = sl::WorkerSettings::Default("localhost:5555");
  
  // Create a worker to communicate with the server
  std::vector<uint> jobList = { 1 };
  sl::comms::Worker worker(jobList, settings);

  // The job spec contains the parameters of the distribution that we are dealing with,
  // namely, the diagonal of the covariance matrix.
  Eigen::VectorXd var = stringToVector(worker.globalSpec());
  sl::stats::DiagNormal norm(Eigen::VectorXd::Zero(var.size()), var);

  // Partial application of the distribution onto the worker function
  auto func = std::bind(workerFunc, norm, ph::_1, ph::_2, ph::_3);

  // Launch a minion to do work
  std::vector<std::future<void>> threads;
  for (int i = 0; i < 1; i++)
  {
    threads.push_back(std::move(sl::runMinionThreaded(worker, 0, func)));
  }

  // Wait for all the threads to finish
  for (int i = 0; i < 1; i++)
  {
    threads[i].wait();
  }

  return 0;
}
