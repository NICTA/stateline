//!
//! Main entry point for using stateline -- server side
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
#include "jsonsettings.hpp"
#include "api.hpp"
#include "../infer/adaptive.hpp"
#include "../infer/chainarray.hpp"
#include "../infer/sampler.hpp"
#include "../comms/delegator.hpp"

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

namespace stateline
{
  struct StatelineSettings
  {
      uint ndims;
      uint nstacks;
      // uint nchains;
      uint ntemps;
      // uint annealLength;
      uint nsamples;
      uint swapInterval;
      // int msLoggingRefresh;
      uint msLoggingRefresh;
      uint nJobTypes;

      bool useInitial;

      Eigen::VectorXd initial;

      // mcmc::SlidingWindowSigmaSettings sigmaSettings;
      // mcmc::SlidingWindowBetaSettings betaSettings;
      double optimalAcceptRate;
      double optimalSwapRate;
      std::string outputPath;
      // mcmc::ChainSettings chainSettings;
      mcmc::ProposalBounds proposalBounds;
      

      static StatelineSettings fromJSON(const nlohmann::json& j)
      {
        StatelineSettings s;
        s.nstacks = readSettings<uint>(j, "nStacks");
        s.ntemps = readSettings<uint>(j, "nTemperatures");
        s.nsamples = readSettings<uint>(j, "nSamplesTotal");
        s.swapInterval = readSettings<uint>(j,"swapInterval");
        s.msLoggingRefresh = (uint)(readSettings<double>(j, "loggingRateSec")*1000.0);
        s.nJobTypes = readSettings<uint>(j, "nJobTypes");
        s.outputPath = readSettings<std::string>(j, "outputPath");
        s.optimalAcceptRate = readSettings<double>(j, "optimalAcceptRate");
        s.optimalSwapRate = readSettings<double>(j, "optimalSwapRate");
        s.useInitial = readSettings<bool>(j, "useInitial");

        if (s.useInitial)
        {
            std::vector<double> tmp;
            tmp = readSettings<std::vector<double>>(j, "initial");
            uint nDims = tmp.size();
            s.initial.resize(nDims);
            for (uint i=0; i < nDims; ++i)
                s.initial[i] = tmp[i];
        }

        s.proposalBounds = mcmc::ProposalBoundsFromJSON(j);
        s.ndims = (uint)s.proposalBounds.min.size(); //ProposalBounds checks they're the same
        return s;
      }
  };

  class ServerWrapper
  {

    public:
      ServerWrapper(uint port, const StatelineSettings& s);
      ~ServerWrapper();
      void start();
      void stop();
      bool isRunning();

    private:
      StatelineSettings settings_;
      bool running_;
      zmq::context_t* context_;
      ApiResources api_;
      comms::Delegator delegator_;
      std::future<void> serverThread_;
      std::future<void> samplerThread_;
      std::future<void> apiServerThread_;
  };
}
