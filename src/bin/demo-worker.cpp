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
#include <json.hpp>
#include <fstream>

#include "app/commandline.hpp"
#include "app/logging.hpp"
#include "app/signal.hpp"
#include "stats/normal.hpp"
#include "comms/minion.hpp"
#include "comms/worker.hpp"

namespace sl = stateline;
namespace po = boost::program_options;
namespace ph = std::placeholders;
using json = nlohmann::json;

po::options_description commandLineOptions()
{
  auto opts = po::options_description("Demo Options");
  opts.add_options()
    ("loglevel,l", po::value<int>()->default_value(0), "Logging level")
    ("address,a",po::value<std::string>()->default_value("localhost:5555"), "Address of server")
    ("config,c",po::value<std::string>()->default_value("config.json"), "Path to configuration file")
    ;
  return opts;
}

json initConfig(const po::variables_map& vm)
{
  std::ifstream configFile(vm["config"].as<std::string>());
  if (!configFile)
  {
    // TODO: use default settings?
    LOG(FATAL) << "Could not find config file";
  }

  json config;
  configFile >> config;
  return config;
}

void runClient(zmq::context_t& context, const po::variables_map& vm, bool& running)
{
  auto settings = sl::comms::WorkerSettings::Default(vm["address"].as<std::string>());
  sl::comms::Worker worker(context, settings, running);
  worker.start();
}

void runWorker(zmq::context_t& context, const po::variables_map& vm, bool& running)
{
  json config = initConfig(vm);

  std::vector<std::string> jobTypes = config["jobTypes"];
  sl::comms::Minion minion(context, jobTypes);

  // Create the log likelihood function of the target distribution.
  auto nll = [&](const Eigen::VectorXd& x)
  {
    return x.squaredNorm();
  };

  // --------------------------------------------------------------------------
  // Launch minion to perform work
  // --------------------------------------------------------------------------
  while (running)
  {
    auto job = minion.nextJob();
    auto jobType = job.first;
    auto sample = job.second;

    // Compute the nll
    minion.submitResult(nll(sample));
  }
}

int main(int ac, char *av[])
{
  zmq::context_t* context = new zmq::context_t{1};

  po::variables_map vm = sl::parseCommandLine(ac, av, commandLineOptions());

  sl::initLogging("client", vm["loglevel"].as<int>(), true, "");
  sl::init::initialiseSignalHandler();

  bool running = true;
  LOG(INFO) << "\033[1;31mstarting client in thread\033[0m";
  auto clientFuture = std::async(std::launch::async, runClient, std::ref(*context), std::cref(vm), std::ref(running));
  LOG(INFO) << "started server in thread";
  LOG(INFO) << "\033[1;31mstarting worker in thread\033[0m";
  auto workerFuture = std::async(std::launch::async, runWorker, std::ref(*context), std::cref(vm), std::ref(running));
  LOG(INFO) << "started sampler in thread";

  while(!sl::global::interruptedBySignal)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  running = false;
  delete context;

  clientFuture.wait();
  workerFuture.wait();
}
