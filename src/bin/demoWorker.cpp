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
#include <queue>
#include <boost/program_options.hpp>

#include <chrono>
#include <thread>

#include "app/worker.hpp"

namespace sl = stateline;
namespace ph = std::placeholders;

std::string workerFunc(uint type, std::string &globalData, std::string &jobData)
{
  return "hello!";
}

int main(int ac, char *av[])
{
  sl::WorkerSettings settings = sl::WorkerSettings::Default("localhost:5555");
  
  // Create a worker to communicate with the server
  std::vector<uint> jobList = { 1 };
  sl::comms::Worker worker(jobList, settings);

  // Launch a minion to do work
  sl::runMinion(worker, 1, workerFunc);

  return 0;
}
