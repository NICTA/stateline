#include "server.hpp"

#include <json.hpp>

#include "comms/delegator.hpp"
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
  }

  StatelineSettings StatelineSettings::fromJSON(const nlohmann::json& j)
  {
    StatelineSettings s;
    s.ndims = j["dimensionality"];
    s.nstacks = j["parallelTempering"]["stacks"];
    s.nchains = j["parallelTempering"]["chains"];
    s.swapInterval = j["parallelTempering"]["swapInterval"];
    s.msLoggingRefresh = 1000;
    s.sigmaSettings = mcmc::SlidingWindowSigmaSettings::fromJSON(j);
    s.betaSettings = mcmc::SlidingWindowBetaSettings::fromJSON(j);
    for (std::string const& i : j["jobTypes"])
    {
      s.jobTypes.push_back(i);
    }
    return s;
  }

  struct Server::State
  {
    mcmc::SlidingWindowSigmaAdapter sigmaAdapter;
    mcmc::SlidingWindowBetaAdapter betaAdapter;
    mcmc::GaussianCovProposal proposal;
    mcmc::CovarianceEstimator covEstimator;
    mcmc::Sampler sampler;

    public:
      State(comms::Requester& requester,
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

  Server::Server(uint port, const StatelineSettings& s)
    : port_(port), settings_(s), context_(new zmq::context_t{1}), requester_(*context_)
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

    state_->sampler.setBufferSize(length);
    return runSampler(length);
  }

  void Server::start()
  {
    running_ = true;

    serverThread_ = std::async(std::launch::async, runServer, std::ref(*context_), port_, std::ref(running_));

    mcmc::SlidingWindowSigmaAdapter sigmaAdapter{settings_.nstacks, settings_.nchains, settings_.ndims, settings_.sigmaSettings};
    mcmc::SlidingWindowBetaAdapter betaAdapter{settings_.nstacks, settings_.nchains, settings_.betaSettings};

    // Initialise the chains with initial samples
    mcmc::ChainArray chains(settings_.nstacks, settings_.nchains, 1);

    for (uint i = 0; i < chains.numTotalChains(); i++)
    {
      // Generate a random initial sample
      Eigen::VectorXd sample = Eigen::VectorXd::Random(settings_.ndims);

      // We now use the worker interface to evaluate this initial sample
      requester_.submit(i, settings_.jobTypes, sample);

      auto result = requester_.retrieve();
      double energy = std::accumulate(std::begin(result.second), std::end(result.second), 0.0);

      // Initialise this chain with the evaluated sample
      chains.initialise(i, sample, energy, sigmaAdapter.sigmas()[i], betaAdapter.betas()[i]);
    }

    // Initialise the server state
    // TODO: replace with C++14 make_unique
    state_ = std::unique_ptr<State>(new State(
          requester_,
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
    if (context_)
    {
      delete context_;
      context_ = nullptr; // ALWAYS DO THIS
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
    // Chain ID and corresponding state.
    uint id;
    mcmc::State state;

    mcmc::TableLogger logger(settings_.nstacks, settings_.nchains, settings_.msLoggingRefresh);

    // Samples array for all the coldest chains
    mcmc::SamplesArray samples(settings_.nstacks);

    for (uint i = 0; i < length; i++)
    {
      // Ask the sampler to return the next state of a chain.
      // 'id' is the ID of the chain and 'state' is the next state in that chain.
      try
      {
        std::tie(id, state) = state_->sampler.step(state_->sigmaAdapter.sigmas(), state_->betaAdapter.betas());
      }
      catch (...)
      {
        break;
      }

      state_->sigmaAdapter.update(id, state);
      state_->betaAdapter.update(id, state);

      state_->covEstimator.update(id, state.sample);
      state_->proposal.update(id, state_->covEstimator.covariances()[id]);

      logger.update(id, state,
          state_->sigmaAdapter.sigmas(), state_->sigmaAdapter.acceptRates(),
          state_->betaAdapter.betas(), state_->betaAdapter.swapRates());

      samples.append(id, state);
    }

    return samples;
  }
}
