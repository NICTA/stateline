//! Implementation of the server wrapper.
//!
//! \file app/serverwrapper.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2015, NICTA
//!

#include "serverwrapper.hpp"

#include "infer/sampler.hpp"
#include "infer/adaptive.hpp"
#include "infer/logging.hpp"

namespace stateline {

namespace {

std::vector<double> eigenVectorToStdVector(const Eigen::VectorXd& v)
{
  return {v.data(), v.data() + v.size()};
}

void runServer(comms::Delegator& delegator, bool& running)
{
  delegator.start(running);
}

std::pair<Eigen::VectorXd,double> generateInitialSample(
        const StatelineSettings& s, comms::Requester& requester,
        const mcmc::ProposalBounds& bounds)
{
  Eigen::VectorXd sample;

  // Send a random sample to workers to be evaluated
  if (s.useInitial)
      sample = mcmc::bouncyBounds(s.initial, bounds.min, bounds.max);
  else
      sample = mcmc::bouncyBounds(Eigen::VectorXd::Random(s.ndims), 
              bounds.min, bounds.max);

  requester.submit(0, eigenVectorToStdVector(sample)); // job id zero because we dont care
  const auto result = requester.retrieve();
  const double energy = std::accumulate(std::begin(result.second), std::end(result.second), 0.0);
  return {sample, energy};
}

void runSampler(const StatelineSettings& s, zmq::context_t& ctx, comms::Delegator& delegator, bool& running)
{
  // Allocate adapters and proposal
  const double max_log_ratio = 4.;  // or would 10 be a better range
  const double min_log_ratio = -8.;
  const std::size_t initial_count = 1000; // TODO: make these settings
  mcmc::RegressionAdapter sigmaAdapter(s.nstacks, s.ntemps, s.optimalAcceptRate, min_log_ratio, max_log_ratio);
  mcmc::RegressionAdapter betaAdapter(s.nstacks, s.ntemps, s.optimalSwapRate, 0., max_log_ratio);
  mcmc::GaussianProposal proposal(s.nstacks, s.ntemps, s.ndims, s.proposalBounds, initial_count);
  mcmc::ChainArray chains(s.nstacks, s.ntemps, s.db);
  comms::Requester requester{ctx, delegator.settings().requesterAddress};

  // Initialise chains to valid states
  // TODO(AL) This loop could be parallelised, but it would take care...
  for (uint i = 0; i < s.nstacks * s.ntemps; i++)
  {

    // Draw an initial sample and compute its energy
    Eigen::VectorXd sample;
    double energy;
    std::tie(sample, energy) = generateInitialSample(s,requester,
            s.proposalBounds);

    // Init betas
    if (i % s.ntemps == 0)
        betaAdapter.computeBetaStack(i);

    // Initialise this chain with the evaluated sample
    chains.initialise(i, sample, energy, sigmaAdapter.values()[i],
            betaAdapter.values()[i]);

    LOG(INFO) << "Initialising chain " << i << " with energy: " << energy
        << "sigma: " << sigmaAdapter.values()[i] << " and beta "
        << betaAdapter.values()[i];

  }

  // Create job types from 0 to max number of job types
  std::vector<uint> jobTypes(s.nJobTypes);
  std::iota(jobTypes.begin(), jobTypes.end(), 0);

  mcmc::Sampler sampler(requester, chains, proposal, sigmaAdapter,
          betaAdapter, s.swapInterval);

  mcmc::TableLogger logger(s.nstacks, s.ntemps, s.ndims, s.loggingRateSec * 1000.0);

  // Chain ID and corresponding state.
  uint id;
  mcmc::State state;

  // Main Loop
  // TODO(Al) confirm that sampler.step does not have to be thread-safe
  //          as I believe there is only one main loop running.
  uint nsamples = 0;
  while (nsamples < s.nsamples && running)
  {
    try
    {
      std::tie(id, state) = sampler.step();
      // 'id' is the ID of the chain and 'state' is its newest state.
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

    // All the adaption can now be found in sampler.step

    // Log the update
    logger.update(id, state,
        sigmaAdapter.values(), sigmaAdapter.rates(),
        betaAdapter.values(), betaAdapter.rates());
  }

  // Finish any outstanding jobs
  LOG(INFO) << "Finished MCMC job with " << nsamples << " samples.";
  sampler.flush();
  if (running == false)
      LOG(INFO) << "Running == False";
  running = false;
}

comms::DelegatorSettings delegatorSettings(int port, const StatelineSettings& s)
{
  comms::DelegatorSettings del{"ipc://stateline.socket", "tcp://*:" + std::to_string(port)};
  del.numJobTypes = s.nJobTypes;
  del.heartbeatTimeout = std::chrono::seconds{s.heartbeatTimeoutSec};
  return del;
}

} // namespace

ServerWrapper::ServerWrapper(int port, const StatelineSettings& s)
  : settings_{s}
  , running_{false}
  , context_{1}
  , delegator_{context_, delegatorSettings(port, s)}
{
}

ServerWrapper::~ServerWrapper()
{
  stop();
}

void ServerWrapper::start()
{
  running_ = true;

  serverThread_ = std::async(std::launch::async, runServer, std::ref(delegator_), std::ref(running_));
  samplerThread_ = std::async(std::launch::async, runSampler, std::cref(settings_),
      std::ref(context_), std::ref(std::ref(delegator_)), std::ref(running_));
}

void ServerWrapper::stop()
{
  running_ = false;

  // Wait for futures to finish
  serverThread_.wait();
  samplerThread_.wait();
}

bool ServerWrapper::isRunning() const
{
  return running_;
}

}
