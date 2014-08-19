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
namespace ch = std::chrono;

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


  uint sigmaWindowSize=100000;
  double coldSigma=0.0001;
  double sigmaFactor=1.5;
  uint sigmaAdaptionLength=100000;
  uint sigmaNStepsPerAdapt=250;
  double sigmaOptimalAccept=0.24;
  double sigmaAdaptRate=0.2;
  double sigmaMinFactor=0.8;
  double sigmaMaxFactor=1.25;

  uint betaWindowSize=100000;
  double betaFactor=0.66;
  uint betaAdaptionLength=100000;
  uint betaNStepsPerAdapt=500;
  double betaOptimalSwapRate=0.24;
  double betaAdaptRate=0.2;
  double betaMinFactor=0.8;
  double betaMaxFactor=1.25;

  // Initialise the parameters of the distribution we are sampling from
  Eigen::MatrixXd means(ncomponents, ndims);
  means << -5, -5, -5,
            5,  5,  5;
 
  // Set up the problem
  sl::mcmc::ProblemInstance problem;
  problem.globalJobSpecData = sl::serialise(means);
  problem.jobConstructFn = sl::mcmc::singleJobConstruct;
  problem.resultLikelihoodFn = sl::mcmc::singleJobLikelihood;
  problem.proposalFn = std::bind(sl::mcmc::adaptiveGaussianProposal, ph::_1, ph::_2,
      Eigen::VectorXd::Constant(ndims, -10),
      Eigen::VectorXd::Constant(ndims, 10));

  // Set up the settings
  sl::mcmc::SamplerSettings settings;
  settings.mcmc = sl::MCMCSettings::Default(nstacks, nchains);
  settings.db = sl::DBSettings::Default();
  settings.del = sl::DelegatorSettings::Default(5555);

  // Generate initial parameters
  std::vector<Eigen::VectorXd> initial;

  for (uint i = 0; i < nstacks; i++)
  {
    for (uint j = 0; j < nchains; j++)
    {
      initial.push_back(Eigen::VectorXd::Random(ndims));
    }
  }

  // Create an adaption system for sigma
  sl::mcmc::SlidingWindowSigmaAdapter sigmaAdapter(nstacks,nchains, ndims,
      sigmaWindowSize,coldSigma,sigmaFactor, sigmaAdaptionLength,
      sigmaNStepsPerAdapt, sigmaOptimalAccept,
      sigmaAdaptRate, sigmaMinFactor, sigmaMaxFactor);

  // Create an adaption system for beta
  sl::mcmc::SlidingWindowBetaAdapter betaAdapter(nstacks, nchains, betaWindowSize,
      betaFactor, betaAdaptionLength, betaNStepsPerAdapt, betaOptimalSwapRate,
      betaAdaptRate, betaMinFactor, betaMaxFactor);
  
  // define initial parameters
  std::vector<Eigen::VectorXd> sigmas = sigmaAdapter.sigmas();
  std::vector<double> betas = betaAdapter.betas();

  // Create a sampler
  sl::mcmc::Sampler sampler(problem, settings, initial, sigmas, betas);
  
   // Record the starting time of the MCMC
  auto startTime = ch::steady_clock::now();
  while ((std::size_t)(ch::duration_cast<ch::seconds>(
          ch::steady_clock::now() - startTime).count()) < numSeconds)
  {
    uint id;
    sl::mcmc::State state;
    std::tie(id, state) = sampler.step(sigmas, betas);
    sigmaAdapter.update(id, state);
    betaAdapter.update(id, state);
    sigmas = sigmaAdapter.sigmas();
    betas = betaAdapter.betas();
  }

  // Output the coldest chains to CSV
  std::ofstream out("output_chain.csv");

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
