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
#include <iomanip>
#include <limits>
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


class Logger
{
  public:
    Logger(uint nstacks, uint nchains, uint msRefresh, 
        const std::vector<Eigen::VectorXd>& sigmas, const std::vector<double>& acceptRates,
        const std::vector<double>& betas, const std::vector<double>& swapRates)
      : msRefresh_(msRefresh),
      nstacks_(nstacks), 
      nchains_(nchains), 
      lengths_(nstacks*nchains),
      minEnergies_(nstacks*nchains),
      energies_(nstacks*nchains),
      nAcceptsGlobal_(nstacks*nchains),
      nSwapsGlobal_(nstacks*nchains),
      nSwapAttemptsGlobal_(nstacks*nchains),
      sigmas_(sigmas),
      acceptRates_(acceptRates),
      betas_(betas),
      swapRates_(swapRates)
  {
    std::fill(lengths_.begin(), lengths_.end(), 1);
    std::fill(minEnergies_.begin(), minEnergies_.end(), std::numeric_limits<double>::infinity());
    std::fill(nAcceptsGlobal_.begin(), nAcceptsGlobal_.end(), 1);
    std::fill(nSwapAttemptsGlobal_.begin(), nSwapAttemptsGlobal_.end(), 1); // for nan-free behaviour
  }

    void update(uint id, const sl::mcmc::State & s)
    {
      // update the variables
      lengths_[id] += 1;
      minEnergies_[id] = std::min(minEnergies_[id], s.energy);
      energies_[id] = s.energy;
      nAcceptsGlobal_[id] += s.accepted;
      nSwapsGlobal_[id] += s.swapType == sl::mcmc::SwapType::Accept;
      nSwapAttemptsGlobal_[id] += s.swapType != sl::mcmc::SwapType::NoAttempt;

      if (ch::duration_cast<ch::milliseconds>(ch::steady_clock::now() - lastPrintTime_).count() > msRefresh_)
        print();
    }

    void print()
    {
      // print to the screen
      lastPrintTime_ = ch::steady_clock::now();
      std::stringstream s;
      s << "\n\nID   Length   MinEngy     CurrEngy     Sigma   AcptRt     GlbAcptRt   Beta        SwapRt   GlbSwapRt\n";
      s << "-----------------------------------------------------------------------------------------------------\n";
      for (uint i = 0; i < lengths_.size(); i++)
      {
        if (i % nchains_ == 0 && i != 0)
          s << '\n';
        s << std::setprecision(6) << std::showpoint << i << " " << std::setw(9) << lengths_[i] << " " << std::setw(10)
          << minEnergies_[i] << " " << std::setw(10) << energies_[i] << " " << std::setw(10) << sigmas_[i](0) << " "
          << std::setw(10) << acceptRates_[i] << " " << std::setw(10) << nAcceptsGlobal_[i] / (double) lengths_[i] << " "
          << std::setw(10) << betas_[i] << " " << std::setw(10) << swapRates_[i] << " " << std::setw(10)
          << nSwapsGlobal_[i] / (double) nSwapAttemptsGlobal_[i] << " \n";
      }
      std::cout << s.str() << std::endl;
    }

  private:

    ch::steady_clock::time_point lastPrintTime_;
    uint msRefresh_;
    uint nstacks_;
    uint nchains_;
    std::vector<uint> lengths_;
    std::vector<double> minEnergies_;
    std::vector<double> energies_;
    std::vector<uint> nAcceptsGlobal_;
    std::vector<uint> nSwapsGlobal_;
    std::vector<uint> nSwapAttemptsGlobal_;
    const std::vector<Eigen::VectorXd>& sigmas_;
    const std::vector<double>& acceptRates_;
    const std::vector<double>& betas_;
    const std::vector<double>& swapRates_;
};

int main(int ac, char *av[])
{
  // Initialise logging
  sl::initLogging("server", 0, true, "");

  const std::size_t ncomponents = 2;
  const std::size_t ndims = 3;
  const std::size_t nchains = 10;
  const std::size_t nstacks = 2;
  const std::size_t numSeconds = 60;
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
 
  // Set up the problem
  sl::mcmc::WorkerInterface workerInterface(sl::serialise(means),
      {}, // empty per-job map
      sl::mcmc::singleJobConstruct,
      sl::mcmc::singleJobLikelihood,
      sl::DelegatorSettings::Default(5555));
     
  
  auto proposalFn = std::bind(sl::mcmc::adaptiveGaussianProposal, ph::_1, ph::_2,
                              Eigen::VectorXd::Constant(ndims, -10),
                              Eigen::VectorXd::Constant(ndims, 10));
  
  // Create a chain array
  sl::mcmc::ChainArray chains(nstacks, nchains, sl::mcmc::ChainSettings::Default());
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

  
  // Create a sampler
  sl::mcmc::Sampler sampler(workerInterface, chains, proposalFn, swapInterval);


  // Create a diagnostic
  sl::mcmc::EPSRDiagnostic diagnostic(nstacks, nchains, ndims);

  // Create a logger
  Logger log(nstacks, nchains, msRefresh, sigmas, acceptRates, betas, swapRates);
  
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
    acceptRates = sigmaAdapter.acceptRates();
    betas = betaAdapter.betas();
    swapRates = betaAdapter.swapRates();
    log.update(id, state);
    diagnostic.update(id, state);
  }

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

