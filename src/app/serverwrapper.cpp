#include "serverwrapper.hpp"
#include "../infer/sampler.hpp"
#include "../infer/adaptive.hpp"
#include "../infer/logging.hpp"

namespace stateline
{
  namespace
  {
    void updateWorkerApi(ApiResources& api, comms::Delegator& delegator)
    {
      api.set("workers", json({{ "count", delegator.workerCount() }}));
    }
  }

  void runServer(comms::Delegator& delegator)
  {
    delegator.start();
  }

  std::pair<Eigen::VectorXd,double> generateInitialSample(
          const StatelineSettings& s, comms::Requester& requester,
          const mcmc::ProposalBounds& bounds)
  {
    Eigen::VectorXd sample;

    std::vector<uint> jobTypes(s.nJobTypes);
    std::iota(jobTypes.begin(), jobTypes.end(), 0);

  // Send a random sample to workers to be evaluated
    sample = mcmc::bouncyBounds(Eigen::VectorXd::Random(s.ndims), 
            bounds.min, bounds.max);
    requester.submit(0, jobTypes, sample); //job id zero because we dont care
    auto result = requester.retrieve();
    double energy = std::accumulate(std::begin(result.second), std::end(result.second), 0.0);
    return {sample, energy};
  }

  void runSampler(const StatelineSettings& s, zmq::context_t& context, ApiResources& api, comms::Delegator& delegator, bool& running)
  {
    mcmc::RegressionAdapter sigmaAdapter(s.nstacks, s.ntemps, s.optimalAcceptRate);
    mcmc::RegressionAdapter betaAdapter(s.nstacks, s.ntemps, s.optimalAcceptRate);
    mcmc::GaussianCovProposal proposal(s.nstacks, s.ntemps, s.ndims, s.proposalBounds);
    mcmc::CovarianceEstimator covEstimator(s.nstacks, s.ntemps, s.ndims);
    comms::Requester requester(context);

    // Create a chain array.
    mcmc::ChainArray chains(s.nstacks, s.ntemps, s.outputPath);

    for (uint i = 0; i < s.nstacks * s.ntemps; i++)
    {
      // Generate the initial sample/energy for this chain
      Eigen::VectorXd sample;
      double energy;
      std::tie(sample,energy) = generateInitialSample(s,requester, s.proposalBounds);
      // Initialise this chain with the evaluated sample
      chains.initialise(i, sample, energy, sigmaAdapter.estimates()[i], betaAdapter.estimates()[i]);
      LOG(INFO) << "Initialising chain " << i << " with energy: " << energy;
    }

    // Create job types from 0 to max number of job types
    std::vector<uint> jobTypes(s.nJobTypes);
    std::iota(jobTypes.begin(), jobTypes.end(), 0);

    // A sampler just takes the worker interface, chain array, proposal function,
    // and how often to attempt swaps.
    mcmc::Sampler sampler(requester, jobTypes, chains, proposal, s.swapInterval);

    mcmc::TableLogger logger(s.nstacks, s.ntemps, s.ndims, s.msLoggingRefresh);

    // Chain ID and corresponding state.
    uint id;
    mcmc::State state;

    uint nsamples = 0;
    while (nsamples < s.nsamples && running)
    {
      // Ask the sampler to return the next state of a chain.
      // 'id' is the ID of the chain and 'state' is the next state in that chain.
      try
      {
        std::tie(id, state) = sampler.step(sigmaAdapter.estimates(), betaAdapter.estimates());
        if (id % s.ntemps == 0)
          nsamples++;
      }
      catch (std::exception const& e)
      {
        LOG(INFO) << "Error in sampler step - aborting:";
        LOG(INFO) << e.what();
        break;
      }
      catch (...)
      {
        LOG(INFO) << "Error in sampler step - aborting:";
        break;
      }

      sigmaAdapter.update(id, state);
      betaAdapter.update(id, state);

      covEstimator.update(id, state.sample);
      proposal.update(id, covEstimator.covariances()[id]);

      logger.update(id, state,
          sigmaAdapter.estimates(), sigmaAdapter.rates(),
          betaAdapter.estimates(), betaAdapter.rates());

      logger.updateApi(api, chains);
      updateWorkerApi(api, delegator);
    }

    // Finish any outstanding jobs
    LOG(INFO) << "Finished MCMC job with " << nsamples << " samples.";
    sampler.flush();
    if (running == false)
        LOG(INFO) << "Running == False";
    running = false;
  }

  ServerWrapper::ServerWrapper(uint port, const StatelineSettings& s)
    : settings_(s)
    , running_(false)
    , context_{new zmq::context_t{1}}
    , delegator_{*context_, comms::DelegatorSettings::Default(port), running_}
  {
  }

  void ServerWrapper::start()
  {
    running_ = true;

    serverThread_ = std::async(std::launch::async, runServer, std::ref(delegator_));
    samplerThread_ = std::async(std::launch::async, runSampler, std::cref(settings_),
        std::ref(*context_), std::ref(api_), std::ref(std::ref(delegator_)), std::ref(running_));
    apiServerThread_ = std::async(std::launch::async, runApiServer, 8080, std::ref(api_), std::ref(running_));
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
