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

#include "app/console.hpp"
#include "app/settings.hpp"
#include "app/async.hpp"
#include "infer/mcmc.hpp"
#include "infer/metropolis.hpp"
#include "infer/adaptive.hpp"

namespace sl = stateline;
namespace ph = std::placeholders;



std::vector<sl::comms::JobData> splitJob(const Eigen::VectorXd &state)
{
  return std::vector<sl::comms::JobData>();
}

double combineResults(const std::vector<sl::comms::ResultData> &results)
{
  return 0.0;
}

int main(int ac, char *av[])
{
  sl::MCMCSettings mcmcSettings = sl::MCMCSettings::Empty();
  
  // Use default db settings
  sl::DBSettings dbSettings = sl::DBSettings::Default();
  
  // Create a delegator to communicate with workers
  sl::DelegatorSettings delSettings = sl::DelegatorSettings::Default(5555);
  sl::comms::Delegator delegator("", { 0 }, { "" }, { "" }, delSettings);

  // Create a policy
  sl::DelegatorAsyncPolicy<> policy(delegator, splitJob, combineResults); 

  // Generate initial states
  std::vector<Eigen::VectorXd> initialStates;
  for (uint i = 0; i < mcmcSettings.chains * mcmcSettings.stacks; i++)
    initialStates.push_back(Eigen::VectorXd::Random(1));

  // Run the MCMC sampler
  bool interrupted = false;
  auto proposal = std::bind(&sl::mcmc::adaptiveGaussianProposal, ph::_1, ph::_2,
      Eigen::VectorXd::Zero(1), Eigen::VectorXd::Zero(1)); 

  sl::mcmc::Sampler sampler(mcmcSettings, dbSettings, 1, interrupted);
  LOG(INFO) << "Starting MCMC run";
  sampler.run(policy, initialStates, proposal, mcmcSettings.wallTime);

  // Get the result chains
  sl::mcmc::ChainArray chains = sampler.chains();

  // Output the final post processed chain
  /*
  io::NpzWriter postProcWriter("outputDemoSimpleMcmcPostproc.npz");
  uint nthin = vm["nthin"].as<uint>();
  uint burnin = vm["burnin"].as<uint>();
  
  // Concatenate the coldest chains
  std::vector<mcmc::State> samples;
  for (uint id = 0; id < chains.numStacks(); id += chains.numChains())
  {
    // Examine the samples, taking into account the burn-in and thinning
    for (uint i = burnin; i < chains.length(id); i += nthin)
    {
      samples.push_back(chains.state(id, i));
    }
  }

  // Perform rejection sampling on the hotter chains
  auto trueDist = [&] (const mcmc::State &state) { return state.energy; };
  auto propDist = [&] (const mcmc::State &state) { return state.energy * state.beta; };
  for (uint id = 0; id < chains.numTotalChains(); id++)
  {
    // Don't take samples from the coldest chains
    if (id % chains.numChains() != 0)
    {
      std::vector<mcmc::State> hotterSamples;

      for (uint i = burnin; i < chains.length(id); i += nthin)
      {
        hotterSamples.push_back(chains.state(id, i));
      }

      // Accept samples from the hotter chains
      std::vector<mcmc::State> acceptedSamples = mcmc::rejectionSample(trueDist, propDist, hotterSamples);
      LOG(INFO) << "Accepted samples from hotter chain with beta = " << chains.beta(id) <<
        ": " << acceptedSamples.size() << "/" << hotterSamples.size() <<
        " (" << acceptedSamples.size() / (double)hotterSamples.size() * 100 << "%)";
      samples.insert(samples.end(), acceptedSamples.begin(), acceptedSamples.end());
    }
  }

  // Convert the final samples into an Eigen vector
  Eigen::VectorXd finalChain(samples.size());

  for (uint i = 0; i < samples.size(); i++)
  {
    finalChain.row(i) = samples[i].sample;
  }

  LOG(INFO) << "Final chain size: " << finalChain.rows();
  postProcWriter.write<double>("chain0", finalChain);*/
}
