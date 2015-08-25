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

  std::pair<Eigen::VectorXd,double> generateInitialSample(const StatelineSettings& s,
                                                          comms::Requester& requester)
  {
    uint n = s.annealLength;

    std::vector<Eigen::VectorXd> sampleVec(n);

    std::vector<uint> jobTypes(s.maxJobTypes);
    std::iota(jobTypes.begin(), jobTypes.end(), 0);

    // Send n random samples to workers to be evaluated
    for (uint i=0; i<n; ++i)
    {
      sampleVec[i] = Eigen::VectorXd::Random(s.ndims);
      requester.submit(i, jobTypes, sampleVec[i]);
    }

    // Retrieve all results and select sample with lowest energy
    uint minEnergyIndex = 0;
    double minEnergy = std::numeric_limits<double>::max();

    for (uint i=0; i<n; ++i)
    {
      auto result = requester.retrieve();
      uint id = result.first;
      double energy = std::accumulate(std::begin(result.second), std::end(result.second), 0.0);
      if (energy < minEnergy)
      {
        minEnergyIndex = id;
        minEnergy = energy;
      }
    }

    return {sampleVec[minEnergyIndex], minEnergy};
  }

  void runSampler(const StatelineSettings& s, zmq::context_t& context, bool& running)
  {
    mcmc::SlidingWindowSigmaAdapter sigmaAdapter(s.nstacks, s.nchains, s.ndims, s.sigmaSettings);
    mcmc::SlidingWindowBetaAdapter betaAdapter(s.nstacks, s.nchains, s.betaSettings);
    mcmc::GaussianCovProposal proposal(s.nstacks, s.nchains, s.ndims, s.proposalBounds);
    mcmc::CovarianceEstimator covEstimator(s.nstacks, s.nchains, s.ndims);
    comms::Requester requester(context);

    // Create a chain array.
    mcmc::ChainArray chains(s.nstacks, s.nchains, s.chainSettings);

    LOG(INFO) << "Initialising chains using annealLength = " << s.annealLength;

    for (uint i = 0; i < s.nstacks * s.nchains; i++)
    {
      // Generate the initial sample/energy for this chain
      Eigen::VectorXd sample;
      double energy;
      std::tie(sample,energy) = generateInitialSample(s,requester);

      // Initialise this chain with the evaluated sample
      chains.initialise(i, sample, energy, sigmaAdapter.sigmas()[i], betaAdapter.betas()[i]);
      LOG(INFO) << "Initialising chain " << i << " with energy: " << energy;
    }

    // Create job types from 0 to max number of job types
    std::vector<uint> jobTypes(s.maxJobTypes);
    std::iota(jobTypes.begin(), jobTypes.end(), 0);

    // A sampler just takes the worker interface, chain array, proposal function,
    // and how often to attempt swaps.
    mcmc::Sampler sampler(requester, jobTypes, chains, proposal, s.swapInterval);

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
