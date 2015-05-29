#include "serverwrapper.hpp" 
#include "../comms/delegator.hpp"
#include "../infer/sampler.hpp"
#include "../infer/adaptive.hpp"
#include "../infer/diagnostics.hpp"
#include "../infer/logging.hpp"

namespace stateline
{
  void runServer(zmq::context_t& context, uint port, bool& running)
  {
    auto settings = comms::DelegatorSettings::Default(port);
    comms::Delegator delegator(context, settings, running);
    delegator.start();
  }

  void runSampler(const StatelineSettings& s, zmq::context_t& context, bool& running)
  {

    mcmc::SlidingWindowSigmaAdapter sigmaAdapter(s.nstacks, s.nchains, s.ndims, s.sigmaSettings);
    mcmc::SlidingWindowBetaAdapter betaAdapter(s.nstacks, s.nchains, s.betaSettings);
    mcmc::GaussianCovProposal proposal(s.nstacks, s.nchains, s.ndims);
    mcmc::CovarianceEstimator covEstimator(s.nstacks, s.nchains, s.ndims);
    comms::Requester requester(context);

    // Create a chain array.
    mcmc::ChainArray chains(s.nstacks, s.nchains, s.chainSettings);


    for (uint i = 0; i < s.nstacks * s.nchains; i++)
    {
      // Generate a random initial sample
      Eigen::VectorXd sample = Eigen::VectorXd::Random(s.ndims);

      // We now use the worker interface to evaluate this initial sample
      requester.submit(i, s.jobTypes, sample);

      auto result = requester.retrieve();
      double energy = std::accumulate(std::begin(result.second), std::end(result.second), 0.0);

      // Initialise this chain with the evaluated sample
      chains.initialise(i, sample, energy, sigmaAdapter.sigmas()[i], betaAdapter.betas()[i]);
    }

    // A sampler just takes the worker interface, chain array, proposal function,
    // and how often to attempt swaps.
    mcmc::Sampler sampler(requester, s.jobTypes, chains, proposal, s.swapInterval);

    mcmc::EPSRDiagnostic diagnostic(s.nstacks, s.nchains, s.ndims);
    mcmc::TableLogger logger(s.nstacks, s.nchains, s.msLoggingRefresh);

    // Record the starting time of the MCMC so we can stop the simulation once
    // the time limit is reached.
    auto startTime = ch::steady_clock::now();

    // Chain ID and corresponding state.
    uint id;
    mcmc::State state;

    while (ch::duration_cast<ch::seconds>(ch::steady_clock::now() - startTime).count() < s.nsecs && running)
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


  ServerWrapper::ServerWrapper(uint port, const StatelineSettings& s)
    : port_(port), settings_(s)
  {
  }

  void ServerWrapper::start()
  {
    running_ = true;
    context_ = new zmq::context_t{1};

    serverThread_ = std::async(std::launch::async, runServer, std::ref(*context_), port_, std::ref(running_));
    samplerThread_ = std::async(std::launch::async, runSampler, std::cref(settings_), 
        std::ref(*context_), std::ref(running_));
  }

  void ServerWrapper::stop()
  {
    running_ = false;
    if (context_)
    {
      delete context_;
      context_ = nullptr; // ALWAYS DO THIS
    }
    // Wait for futures to finish
    serverThread_.wait();
    samplerThread_.wait();
  }

  bool ServerWrapper::isRunning()
  {
    return running_;
  }

  ServerWrapper::~ServerWrapper()
  {
    stop();
  }


}
