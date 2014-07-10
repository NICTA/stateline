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

std::string workerFunc(uint type, const std::string &globalData, const std::string &jobData)
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
  std::vector<std::future<void>> threads;
  for (int i = 0; i < 10; i++)
  {
    threads.push_back(std::move(sl::runMinionThreaded(worker, 0, workerFunc)));
  }

  // Wait for all the threads to finish

  return 0;
}
