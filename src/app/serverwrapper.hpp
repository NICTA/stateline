//! Wrapper around the inference enginer and a delegator.
//!
//! \file app/serverwrapper.hpp
//! \author Lachlan McCalman
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2015, NICTA
//!

#pragma once

#include <future>
#include <zmq.hpp>
#include <json.hpp>

#include "app/json.hpp"
#include "comms/delegator.hpp"
#include "infer/adaptive.hpp"
#include "infer/chainarray.hpp"
#include "infer/sampler.hpp"

// Ideal config file should look like:
// {
// "min": [-10, 0, -10, -10],
// "max": [ 10, 10, 10, 2],
// "nSamplesTotal": 10000,
// "nJobTypes": 3,
// "optimalAcceptRate": 0.234,
// "outputPath": "demo-output",
// "loggingRateSec": 1,
// "nStacks": 2,
// "nTemperatures": 4,
// "swapInterval": 10
// }

namespace stateline {

struct StatelineSettings
{
  std::size_t ndims;
  std::size_t nstacks;

  std::size_t ntemps;
  std::size_t nsamples;
  std::size_t swapInterval;
  std::size_t loggingRateSec;
  comms::JobType nJobTypes;

  bool useInitial;
  Eigen::VectorXd initial;

  double optimalAcceptRate;
  double optimalSwapRate;

  mcmc::ProposalBounds proposalBounds;

  std::size_t heartbeatTimeoutSec;

  std::string outputPath;

  static StatelineSettings fromJSON(const nlohmann::json& j)
  {
    StatelineSettings s;
    readFields(j, "nStacks", s.nstacks);
    readFields(j, "nTemperatures", s.ntemps);
    readFields(j, "nSamplesTotal", s.nsamples);
    readFields(j, "swapInterval", s.swapInterval);
    readFields(j, "loggingRateSec", s.loggingRateSec);
    readFields(j, "nJobTypes", s.nJobTypes);
    readFields(j, "outputPath", s.outputPath);
    readFields(j, "optimalAcceptRate", s.optimalAcceptRate);
    readFields(j, "optimalSwapRate", s.optimalSwapRate);
    readFieldsWithDefault(j, "useInitial", s.useInitial, false);

    if (s.useInitial)
    {
      std::vector<double> initial;
      readFields(j, "initial", initial);

      s.initial.resize(initial.size());
      for (std::size_t i = 0; i < initial.size(); i++)
        s.initial(i) = initial[i];
    }

    readFieldsWithDefault(j, "heartbeatTimeoutSec", s.heartbeatTimeoutSec, 15);

    s.proposalBounds = mcmc::ProposalBoundsFromJSON(j);
    s.ndims = s.proposalBounds.min.size(); //ProposalBounds checks they're the same
    return s;
  }
};

class ServerWrapper
{
public:
  ServerWrapper(int port, const StatelineSettings& s);
  ~ServerWrapper();

  void start();
  void stop();

  bool isRunning() const;

private:
  StatelineSettings settings_;
  bool running_;
  zmq::context_t context_;
  comms::Delegator delegator_;
  std::future<void> serverThread_;
  std::future<void> samplerThread_;
};

}
