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
#include <fstream>
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

int main(int ac, char *av[])
{
  // Initialise logging
  sl::initLogging("server", 0, true, "");

  const std::size_t ncomponents = 2;
  const std::size_t ndims = 3;
  const std::size_t nchains = 5;
  const std::size_t nstacks = 2;
  const std::size_t numSeconds = 60;

  // Initialise the parameters of the distribution we are sampling from
  Eigen::MatrixXd means(ncomponents, ndims);
  means << -5, -5, -5,
            5,  5,  5;

  // Generate initial states
  std::vector<Eigen::VectorXd> initialStates;
  for (uint i = 0; i < nstacks * nchains; i++)
    initialStates.push_back(Eigen::VectorXd::Random(ndims));
 
  // Creating a proposal distribution
  auto proposal = std::bind(sl::mcmc::adaptiveGaussianProposal, ph::_1, ph::_2,
      Eigen::VectorXd::Constant(ndims, -10),
      Eigen::VectorXd::Constant(ndims, 10)); 

  // Set up and start the delegator
  sl::comms::Delegator delegator(sl::serialise(means), {{ 0, "" }},
      sl::DelegatorSettings::Default(5555));
  delegator.start();

  // Create an async policy for the MCMC
  sl::SingleTaskAsyncPolicy policy(delegator);

  // Create a sampler and run it
  sl::mcmc::Sampler sampler(
      sl::MCMCSettings::Default(nchains, nstacks),
      sl::DBSettings::Default(), ndims);
  sl::mcmc::SimpleLogger logger(sampler.settings(), 3);
  sampler.run(policy, initialStates, proposal, numSeconds, logger);

  // Output the coldest chains to CSV
  std::ofstream out("chain.csv");

  std::vector<sl::mcmc::State> states = sampler.chains().states(0);
  for (auto &state : states) 
  {
    for (Eigen::VectorXd::Index i = 0; i < state.sample.size(); i++)
    {
      if (i > 0) out << ",";
      out << state.sample(i);
    }
    out << "\n";
  }

  return 0;
}
