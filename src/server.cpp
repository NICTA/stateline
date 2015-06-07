#include "server.hpp"

#include <json.hpp>

#include "comms/delegator.hpp"
#include "comms/requester.hpp"
#include "infer/sampler.hpp"
#include "infer/adaptive.hpp"
#include "infer/diagnostics.hpp"
#include "infer/logging.hpp"

namespace stateline
{
  namespace
  {
    void runServer(zmq::context_t& context, uint port, bool& running)
    {
      auto settings = comms::DelegatorSettings::Default(port);
      comms::Delegator delegator(context, settings, running);
      delegator.start();
    }

    struct SamplerState
    {
      mcmc::SlidingWindowSigmaAdapter sigmaAdapter;
      mcmc::SlidingWindowBetaAdapter betaAdapter;
      mcmc::GaussianCovProposal proposal;
      mcmc::CovarianceEstimator covEstimator;
      mcmc::Sampler sampler;

      public:
        SamplerState(comms::Requester& requester,
            const std::vector<std::string>& jobTypes,
            const mcmc::ChainArray& chains,
            const mcmc::GaussianCovProposal& proposal,
            uint swapInterval,
            const mcmc::SlidingWindowSigmaAdapter& sigmaAdapter,
            const mcmc::SlidingWindowBetaAdapter& betaAdapter,
            const mcmc::CovarianceEstimator& covEstimator)
          : sigmaAdapter(sigmaAdapter),
            betaAdapter(betaAdapter),
            proposal(proposal),
            covEstimator(covEstimator),
            sampler{requester, jobTypes, chains, proposal, swapInterval}
        {
        }
    };
  }

  StatelineSettings StatelineSettings::fromDefault(uint numDims,
      const std::vector<std::string>& jobTypes,
      uint numStacks, uint numChains)
  {
    StatelineSettings s;
    s.ndims = numDims;
    s.jobTypes = jobTypes;
    s.nstacks = numStacks;
    s.nchains = numChains;
    s.swapInterval = 10;
    s.msLoggingRefresh = 1000;
    s.sigmaSettings = mcmc::SlidingWindowSigmaSettings::fromDefault();
    s.betaSettings = mcmc::SlidingWindowBetaSettings::fromDefault();
    return s;
  }

  struct Server::Impl
  {
    // These two members need to be initialised first
    zmq::context_t* ctx;
    comms::Requester requester;

    // These has to be lazily initialised after the context
    std::unique_ptr<SamplerState> state;

    Impl()
      : ctx(new zmq::context_t{1}), requester{*ctx}
    {
    }
  };

  Server::Server(uint port, const StatelineSettings& s)
    : impl_(new Impl()), port_(port), settings_(s), running_(false)
  {
  }

  Server::~Server()
  {
    stop();
  }

  mcmc::SamplesArray Server::step(uint length)
  {
    // Start the server if it's not yet started
    if (!running_)
    {
      start();
    }

    impl_->state->sampler.setBufferSize(length + 1); // 1 more so the next call works
    return runSampler(length);
  }

  void Server::start()
  {
    running_ = true;

    serverThread_ = std::async(std::launch::async, runServer,
        std::ref(*impl_->ctx), port_, std::ref(running_));

    mcmc::SlidingWindowSigmaAdapter sigmaAdapter{settings_.nstacks, settings_.nchains, settings_.ndims, settings_.sigmaSettings};
    mcmc::SlidingWindowBetaAdapter betaAdapter{settings_.nstacks, settings_.nchains, settings_.betaSettings};

    // Initialise the chains with initial samples
    mcmc::ChainArray chains(settings_.nstacks, settings_.nchains, 2);

    for (uint i = 0; i < chains.numTotalChains(); i++)
    {
      // Generate a random initial sample
      Eigen::VectorXd sample = Eigen::VectorXd::Random(settings_.ndims);

      // We now use the worker interface to evaluate this initial sample
      impl_->requester.submit(i, settings_.jobTypes, sample);

      auto result = impl_->requester.retrieve();
      double energy = std::accumulate(std::begin(result.second), std::end(result.second), 0.0);

      // Initialise this chain with the evaluated sample
      chains.initialise(i, sample, energy, sigmaAdapter.sigmas()[i], betaAdapter.betas()[i]);
    }

    // Initialise the server state
    // TODO: replace with C++14 make_unique
    impl_->state = std::unique_ptr<SamplerState>(new SamplerState(
          impl_->requester,
          settings_.jobTypes,
          chains,
          {settings_.nstacks, settings_.nchains, settings_.ndims},
          settings_.swapInterval,
          sigmaAdapter, betaAdapter,
          {settings_.nstacks, settings_.nchains, settings_.ndims}));
  }

  void Server::stop()
  {
    running_ = false;
    if (impl_->ctx)
    {
      delete impl_->ctx;
      impl_->ctx = nullptr; // ALWAYS DO THIS
    }
    // Wait for futures to finish
    serverThread_.wait();
  }

  bool Server::isRunning()
  {
    return running_;
  }

  mcmc::SamplesArray Server::runSampler(uint length)
  {
    // Avoid typing
    SamplerState& samplerState = *impl_->state;

    // Chain ID and corresponding state.
    uint id;
    mcmc::State state;

    mcmc::TableLogger logger(settings_.nstacks, settings_.nchains, settings_.msLoggingRefresh);

    // Samples array for all the coldest chains
    mcmc::SamplesArray samples(settings_.nstacks);

    uint numIters = length * settings_.nstacks * settings_.nchains;

    for (uint i = 0; i < numIters; i++)
    {
      // Ask the sampler to return the next state of a chain.
      // 'id' is the ID of the chain and 'state' is the next state in that chain.
      try
      {
        std::tie(id, state) = samplerState.sampler.step(samplerState.sigmaAdapter.sigmas(), samplerState.betaAdapter.betas());
      }
      catch (...)
      {
        break;
      }

      samplerState.sigmaAdapter.update(id, state);
      samplerState.betaAdapter.update(id, state);

      samplerState.covEstimator.update(id, state.sample);
      samplerState.proposal.update(id, samplerState.covEstimator.covariances()[id]);

      logger.update(id, state,
          samplerState.sigmaAdapter.sigmas(), samplerState.sigmaAdapter.acceptRates(),
          samplerState.betaAdapter.betas(), samplerState.betaAdapter.swapRates());

      samples.append(id, state);
    }

    return samples;
  }
}
