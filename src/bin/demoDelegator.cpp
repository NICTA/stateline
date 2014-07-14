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

po::options_description commandLineOptions()
{
  auto opts = po::options_description("Demo Options");
  opts.add_options()
    ("nthin,t", po::value<uint>()->default_value(10), "The number of samples discarded for each sample used")
    ("burnin,b", po::value<uint>()->default_value(0), "The number of samples to discard from the beginning");

  return opts;
}

void runMCMCNormal(const Eigen::VectorXd &var, sl::SingleTaskAsyncPolicy &policy)
{
  uint dims = var.size();

  // Initialise MCMC settings
  sl::MCMCSettings mcmcSettings = sl::MCMCSettings::NoAdaption(1, 1, 5);
  
  // Use default db settings
  sl::DBSettings dbSettings = sl::DBSettings::Default();

  // Generate initial states
  std::vector<Eigen::VectorXd> initialStates;
  for (uint i = 0; i < mcmcSettings.chains * mcmcSettings.stacks; i++)
    initialStates.push_back(Eigen::VectorXd::Random(dims));

  // Run the MCMC sampler
  bool interrupted = false;
  auto proposal = std::bind(sl::mcmc::adaptiveGaussianProposal, ph::_1, ph::_2,
      Eigen::VectorXd::Zero(dims), Eigen::VectorXd::Zero(dims)); 

  sl::mcmc::Sampler sampler(mcmcSettings, dbSettings, dims, interrupted);
  LOG(INFO) << "Starting MCMC run";
  sampler.run(policy, initialStates, proposal, mcmcSettings.wallTime);

  // Get the result chains
  const sl::mcmc::ChainArray &chains = sampler.chains();
}

int main(int ac, char *av[])
{
  // Initialise logging
  sl::initLogging("server", -3, true, "");

  // Initialise the parameters of the distribution we are sampling from
  Eigen::VectorXd var(3);
  var << 1, 2, 3;

  // Bind the parameters with the MCMC runner
  auto runMCMC = std::bind(runMCMCNormal, var, ph::_1);
  
  // Run delegator (blocking call)
  sl::DelegatorSettings settings = sl::DelegatorSettings::Default(5555);
  sl::runDelegatorWithPolicy<sl::SingleTaskAsyncPolicy>(runMCMC, sl::serialise(var), settings);

  return 0;
}
