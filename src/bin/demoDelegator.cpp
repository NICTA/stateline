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
#include <limits>
#include <functional>
#include <fstream>
#include <boost/program_options.hpp>

#include <chrono>
#include <thread>

#include "infer/sampler.hpp"
#include "infer/adaptive.hpp"
#include "infer/diagnostics.hpp"
#include "infer/logging.hpp"
#include "app/logging.hpp"
#include "app/serial.hpp"
#include "app/signal.hpp"
#include "app/commandline.hpp"

namespace sl = stateline;
namespace ph = std::placeholders;
namespace po = boost::program_options;
namespace ch = std::chrono;

po::options_description commandLineOptions()
{
  auto opts = po::options_description("Demo Options");
  opts.add_options()
  ("loglevel,l", po::value<int>()->default_value(0), "Logging level")
  ("recover,r", po::bool_switch()->default_value(false), "Recover an existing chain")
  ("port,p",po::value<uint>()->default_value(5555), "Port on which to accept worker connections") 
  ("time,t",po::value<uint>()->default_value(60), "Number of seconds the sampler will run for")
  ;
  return opts;
}

int main(int ac, char *av[])
{
  sl::init::initialiseSignalHandler();
  // Initialise logging
  po::variables_map vm = sl::parseCommandLine(ac, av, commandLineOptions());
  sl::initLogging("server", vm["loglevel"].as<int>(), true, "");

  const std::size_t ncomponents = 2;
  const std::size_t ndims = 3;
  const std::size_t nchains = 10;
  const std::size_t nstacks = 2;
  const std::size_t numSeconds = vm["time"].as<uint>();
  const std::size_t swapInterval = 10;
  uint msRefresh = 500;

  sl::mcmc::SlidingWindowSigmaSettings sigmaSettings = sl::mcmc::SlidingWindowSigmaSettings::Default();
  sl::mcmc::SlidingWindowBetaSettings betaSettings = sl::mcmc::SlidingWindowBetaSettings::Default();
  
  // Create an adaption system for sigma
  sl::mcmc::SlidingWindowSigmaAdapter sigmaAdapter(nstacks,nchains, ndims,
      sigmaSettings);
  // Create an adaption system for beta
  sl::mcmc::SlidingWindowBetaAdapter betaAdapter(nstacks, nchains, betaSettings);
  // define initial parameters
  std::vector<Eigen::VectorXd> sigmas = sigmaAdapter.sigmas();
  std::vector<double> acceptRates = sigmaAdapter.acceptRates();
  std::vector<double> betas = betaAdapter.betas();
  std::vector<double> swapRates = betaAdapter.swapRates();


  // Initialise the parameters of the distribution we are sampling from
  Eigen::MatrixXd means(ncomponents, ndims);
  means << -5, -5, -5,
            5,  5,  5;
 
  uint port = vm["port"].as<uint>();
  // Set up the problem
  sl::mcmc::WorkerInterface workerInterface(sl::serialise(means),
      {}, // empty per-job map
      sl::mcmc::singleJobConstruct,
      sl::mcmc::singleJobEnergy,
      sl::DelegatorSettings::Default(port));
     
  
  auto proposalFn = std::bind(sl::mcmc::truncatedGaussianProposal, ph::_1, ph::_2, ph::_3,
                              Eigen::VectorXd::Constant(ndims, -10),
                              Eigen::VectorXd::Constant(ndims, 10));
  
  // Recover the chain array?
  bool recover = vm["recover"].as<bool>();
  // Create a chain array
  sl::mcmc::ChainArray chains(nstacks, nchains, sl::mcmc::ChainSettings::Default(recover));
  if (!recover)
  {
    // Generate initial samples
    for (uint i = 0; i < nstacks * nchains; i++)
    {
      Eigen::VectorXd sample = Eigen::VectorXd::Random(ndims);
      workerInterface.submit(i, sample);
      uint j;
      double energy;
      std::tie(j, energy) = workerInterface.retrieve();
      chains.initialise(i, sample, energy, sigmas[i], betas[i]);
    }
  }

  // Create a sampler
  sl::mcmc::Sampler sampler(workerInterface, chains, proposalFn, swapInterval);

  // Create a diagnostic
  sl::mcmc::EPSRDiagnostic diagnostic(nstacks, nchains, ndims);

  // Create a logger
  sl::mcmc::Logger log(nstacks, nchains, msRefresh, sigmas, acceptRates, betas, swapRates);
  
   // Record the starting time of the MCMC
  auto startTime = ch::steady_clock::now();
  while ((std::size_t)(ch::duration_cast<ch::seconds>(
          ch::steady_clock::now() - startTime).count()) < numSeconds && !sl::global::interruptedBySignal)
  {
    uint id;
    sl::mcmc::State state;
    std::tie(id, state) = sampler.step(sigmas, betas);
    sigmaAdapter.update(id, state);
    betaAdapter.update(id, state);
    sigmas = sigmaAdapter.sigmas();
    acceptRates = sigmaAdapter.acceptRates();
    betas = betaAdapter.betas();
    swapRates = betaAdapter.swapRates();
    log.update(id, state);
    diagnostic.update(id, state);
  }

  // Finish off outstanding jobs
  sampler.flush();

  std::cout << "convergence: " << diagnostic.rHat().transpose() << std::endl;

  // Output the coldest chains to CSV
  std::ofstream out("output_chain.csv");

  std::vector<sl::mcmc::State> states = chains.states(0);
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

