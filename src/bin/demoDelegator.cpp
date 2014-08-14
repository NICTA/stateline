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

#include "infer/sampler.hpp"
#include "infer/adaptive.hpp"
#include "app/logging.hpp"

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
 
  // Set up the problem
  sl::mcmc::ProblemInstance problem;
  problem.globalJobSpecData = sl::serialise(means);
  problem.jobConstructFn = sl::mcmc::singleJobConstruct;
  problem.resultLikelihoodFn = sl::mcmc::singleJobLikelihood;
  problem.proposalFn = std::bind(sl::mcmc::reflectiveGaussianProposal, ph::_1, ph::_2,
      Eigen::VectorXd::Constant(ndims, -10),
      Eigen::VectorXd::Constant(ndims, 10));

  // Set up the settings
  sl::mcmc::SamplerSettings settings;
  settings.mcmc = sl::MCMCSettings::Default(nstacks, nchains);
  settings.db = sl::DBSettings::Default();
  settings.del = sl::DelegatorSettings::Default(5555);

  // Generate initial parameters
  std::vector<Eigen::VectorXd> initial;
  std::vector<Eigen::VectorXd> sigmas;
  std::vector<double> betas;

  for (uint i = 0; i < nstacks; i++)
  {
    for (uint j = 0; j < nchains; j++)
    {
      initial.push_back(Eigen::VectorXd::Random(ndims));
      sigmas.push_back(Eigen::VectorXd::Ones(ndims));
      betas.push_back(1.0 / std::pow(2.1, j));
    }
  }

  // Create a sampler and run it
  sl::mcmc::Sampler sampler(problem, settings, initial, sigmas, betas);
  while (true) // TODO: add stopping criteria
  {
    // TODO: adapt and survive
    sampler.step(sigmas, betas);
  }

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
