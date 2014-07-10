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
#include <boost/program_options.hpp>

#include <chrono>
#include <thread>

#include "app/logging.hpp"
#include "app/delegator.hpp"
#include "infer/mcmc.hpp"
#include "infer/metropolis.hpp"
#include "infer/adaptive.hpp"

namespace sl = stateline;
namespace ph = std::placeholders;
namespace po = boost::program_options;

std::string vectorToString(const Eigen::VectorXd &state)
{
  std::string buffer = std::to_string(state.size());
  for (Eigen::VectorXd::Index i = 0; i < state.size(); i++)
  {
    buffer += " " + std::to_string(state(i));
  }
  std::cout << "buffer: " << buffer << std::endl;
  return buffer;
}

std::vector<sl::comms::JobData> splitJob(const Eigen::VectorXd &state)
{
  // Send the state vector as a single job
  sl::comms::JobData job;
  job.type = 0;
  job.globalData = "";
  job.jobData = vectorToString(state);

  return { job };
}

double combineResults(const std::vector<sl::comms::ResultData> &results)
{
  // We only expect one result
  return std::stod(results.front().data);
}

po::options_description commandLineOptions()
{
  auto opts = po::options_description("Demo Options");
  opts.add_options()
    ("nthin,t", po::value<uint>()->default_value(10), "The number of samples discarded for each sample used")
    ("burnin,b", po::value<uint>()->default_value(0), "The number of samples to discard from the beginning");

  return opts;
}

int main(int ac, char *av[])
{
  // Initialise logging
  sl::initLogging("server", -3, true, "");

  // Initialise MCMC settings
  sl::MCMCSettings mcmcSettings = sl::MCMCSettings::NoAdaption(1, 1, 5);
  
  // Use default db settings
  sl::DBSettings dbSettings = sl::DBSettings::Default();

  // Initialise the parameters of the distribution we are sampling from
  uint dims = 3;
  Eigen::VectorXd var(dims);
  var << 1, 1, 1;
  
  // Create a delegator to communicate with workers
  sl::DelegatorSettings delSettings = sl::DelegatorSettings::Default(5555);
  sl::comms::Delegator delegator(vectorToString(var), { 0 }, {""}, delSettings);
  delegator.start();

  // Create a policy
  sl::DelegatorAsyncPolicy<> policy(delegator, splitJob, combineResults); 

  // Generate initial states
  std::vector<Eigen::VectorXd> initialStates;
  for (uint i = 0; i < mcmcSettings.chains * mcmcSettings.stacks; i++)
    initialStates.push_back(Eigen::VectorXd::Random(dims));

  // Run the MCMC sampler
  bool interrupted = false;
  auto proposal = std::bind(sl::mcmc::adaptiveGaussianProposal, ph::_1, ph::_2,
      Eigen::VectorXd::Zero(var.size()), Eigen::VectorXd::Zero(var.size())); 

  sl::mcmc::Sampler sampler(mcmcSettings, dbSettings, dims, interrupted);
  LOG(INFO) << "Starting MCMC run";
  sampler.run(policy, initialStates, proposal, mcmcSettings.wallTime);

  // Get the result chains
  sl::mcmc::ChainArray chains = sampler.chains();
}
