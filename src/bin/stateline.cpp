//!
//! A demo using Stateline to sample from a Gaussian mixture.
//!
//! \file stateline.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \licence Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <iostream>
#include <functional>
#include <fstream>
#include <boost/program_options.hpp>
#include <json.hpp>

#include <chrono>

#include "../comms/delegator.hpp"
#include "../infer/sampler.hpp"
#include "../infer/adaptive.hpp"
#include "../infer/diagnostics.hpp"
#include "../infer/logging.hpp"
#include "../app/logging.hpp"
#include "../app/serial.hpp"
#include "../app/signal.hpp"
#include "../app/commandline.hpp"

// Alias namespaces for conciseness
namespace sl = stateline;
namespace ph = std::placeholders;
namespace po = boost::program_options;
namespace ch = std::chrono;
using json = nlohmann::json;

po::options_description commandLineOptions()
{
  auto opts = po::options_description("Demo Options");
  opts.add_options()
  ("loglevel,l", po::value<int>()->default_value(0), "Logging level")
  ("port,p",po::value<uint>()->default_value(5555), "Port on which to accept worker connections") 
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

void runServer(zmq::context_t& context, const po::variables_map& vm, bool& running)
{
  auto settings = sl::comms::DelegatorSettings::Default(vm["port"].as<uint>());
  sl::comms::Delegator delegator(context, settings, running);
  delegator.start();
}

void runSampler(zmq::context_t& context, const po::variables_map& vm, bool& running)
{
  json config = initConfig(vm);

  const uint ndims = config["dimensionality"];
  const uint nstacks = config["parallelTempering"]["stacks"];
  const uint nchains = config["parallelTempering"]["chains"];
  const uint nsecs = config["duration"];
  const uint swapInterval = config["parallelTempering"]["swapInterval"];
  const int msLoggingRefresh = 1000;

  // Stateline focuses on Metropolis-Hastings samplers, which involve a
  // proposal function that generates the next state in the MCMC chain.
  // The term 'sigma' is used liberally as a measure of 'step size', the
  // distance moved between consecutive states. A high sigma indicates larger
  // jumps, which may result in faster mixing, but higher rejection rates.
  // A low sigma indicates smaller steps, which lead to strong correlation
  // between samples, but higher acceptance rates.
  //
  // Here, we want an adaptive sigma, which varies the step size depending on
  // the acceptance rate.
  sl::mcmc::SlidingWindowSigmaSettings sigmaSettings(config);
  sl::mcmc::SlidingWindowSigmaAdapter sigmaAdapter(nstacks, nchains, ndims, sigmaSettings);

  // Stateline uses parallel-tempering by default, which involves running
  // chains within a stack at different 'temperatures'. At higher temperatures,
  // chains tend to mix faster. Every so often, chains within a stack will
  // attempt to swap states, which allows the colder chains to mix faster as a
  // result. Here, 'beta' is the inverse of the temperature value.
  //
  // Create an adaptive beta system, which varies the temperatures based on the
  // swap rates of various chains.
  sl::mcmc::SlidingWindowBetaSettings betaSettings(config);
  sl::mcmc::SlidingWindowBetaAdapter betaAdapter(nstacks, nchains, betaSettings);

  // --------------------------------------------------------------------------
  // Initialise the proposal function
  // --------------------------------------------------------------------------

  // Since the distribution we are sampling from is Gaussian, we can use a
  // multivariate Gaussian as a proposal distribution to mix faster.
  sl::mcmc::GaussianCovProposal proposal(nstacks, nchains, ndims);

  // We'll use the sample covariance of the chain as the covariance matrix
  // of the proposal distribution. To do this, we create a covariance estimator
  // that efficiently computes the sample covariance.
  sl::mcmc::CovarianceEstimator covEstimator(nstacks, nchains, ndims);

  // --------------------------------------------------------------------------
  // Initialise the requester
  // --------------------------------------------------------------------------
  sl::comms::Requester requester(context);

  // --------------------------------------------------------------------------
  // Set up the chain array
  // --------------------------------------------------------------------------

  // Create a chain array.
  sl::mcmc::ChainSettings chainSettings;
  chainSettings.databasePath = config["output"]["directory"].get<std::string>();
  chainSettings.chainCacheLength = config["output"]["cacheLength"];
  sl::mcmc::ChainArray chains(nstacks, nchains, chainSettings);

  std::vector<std::string> jobTypes = config["jobTypes"];

  for (uint i = 0; i < nstacks * nchains; i++)
  {
    // Generate a random initial sample
    Eigen::VectorXd sample = Eigen::VectorXd::Random(ndims);

    // We now use the worker interface to evaluate this initial sample
    requester.submit(i, jobTypes, sample);

    auto result = requester.retrieve();
    double energy = std::accumulate(std::begin(result.second), std::end(result.second), 0.0);

    // Initialise this chain with the evaluated sample
    chains.initialise(i, sample, energy, sigmaAdapter.sigmas()[i], betaAdapter.betas()[i]);
  }

  // --------------------------------------------------------------------------
  // Create a sampler and various other components
  // --------------------------------------------------------------------------

  // A sampler just takes the worker interface, chain array, proposal function,
  // and how often to attempt swaps.
  sl::mcmc::Sampler sampler(requester, jobTypes, chains, proposal, swapInterval);

  // Stateline offers various auxiliary classes used for diagnostics and logging.
  // Here, we create a convergence test and standard output logging.
  sl::mcmc::EPSRDiagnostic diagnostic(nstacks, nchains, ndims);
  sl::mcmc::TableLogger logger(nstacks, nchains, msLoggingRefresh);

  // --------------------------------------------------------------------------
  // Run the MCMC
  // --------------------------------------------------------------------------

  // Record the starting time of the MCMC so we can stop the simulation once
  // the time limit is reached.
  auto startTime = ch::steady_clock::now();

  // Chain ID and corresponding state.
  uint id;
  sl::mcmc::State state;

  while (ch::duration_cast<ch::seconds>(ch::steady_clock::now() - startTime).count() < nsecs &&
      !sl::global::interruptedBySignal)
  {
    // Ask the sampler to return the next state of a chain.
    // 'id' is the ID of the chain and 'state' is the next state in that chain.
    try
    {
      std::tie(id, state) = sampler.step(sigmaAdapter.sigmas(), betaAdapter.betas());
    }
    catch (...)
    {
      break;
    }

    // In most set ups, the main MCMC loop will need to update the various
    // objects used by the sampler. The arguments passed to the update method
    // varies depending on the class, so you should consult the documentation.
    sigmaAdapter.update(id, state);
    betaAdapter.update(id, state);

    covEstimator.update(id, state.sample);
    proposal.update(id, covEstimator.covariances()[id]);

    logger.update(id, state,
        sigmaAdapter.sigmas(), sigmaAdapter.acceptRates(),
        betaAdapter.betas(), betaAdapter.swapRates());

    diagnostic.update(id, state);
  }

  // Finish any outstanding jobs
  sampler.flush();

  running = false;
}

int main(int ac, char *av[])
{
  zmq::context_t* context = new zmq::context_t{1};

  po::variables_map vm = sl::parseCommandLine(ac, av, commandLineOptions());

  sl::init::initialiseSignalHandler();
  sl::initLogging("server", vm["loglevel"].as<int>(), true, "");

  bool running = true;
  LOG(INFO) << "\033[1;31mstarting server in thread\033[0m";
  auto serverFuture = std::async(std::launch::async, runServer, std::ref(*context), std::cref(vm), std::ref(running));
  LOG(INFO) << "started server in thread";
  LOG(INFO) << "\033[1;31mstarting sampler in thread\033[0m";
  auto samplerFuture = std::async(std::launch::async, runSampler, std::ref(*context), std::cref(vm), std::ref(running));
  LOG(INFO) << "started sampler in thread";

  while(!sl::global::interruptedBySignal && running)
  {
    std::this_thread::sleep_for(ch::milliseconds(500));
  }

  running = false;
  delete context;

  // Wait for futures to finish
  serverFuture.wait();
  samplerFuture.wait();
}
