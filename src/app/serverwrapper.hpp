#pragma once
//!
//! Main entry point for using stateline -- server side
//!
//! 
//!
//! \file app/serverwrapper.hpp
//! \author Lachlan McCalman
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2015, NICTA
//!

#include <future>
#include <zmq.hpp>
#include <json.hpp>
#include "jsonsettings.hpp"
#include "../infer/adaptive.hpp"
#include "../infer/chainarray.hpp"
#include "../infer/sampler.hpp"

namespace stateline
{
  struct StatelineSettings
  {
      uint ndims;
      uint nstacks;
      uint nchains;
      uint annealLength;
      uint nsamples;
      uint swapInterval;
      int msLoggingRefresh;
      mcmc::SlidingWindowSigmaSettings sigmaSettings;
      mcmc::SlidingWindowBetaSettings betaSettings;
      mcmc::ChainSettings chainSettings;
      uint nJobTypes;
      mcmc::ProposalBounds proposalBounds;

      static StatelineSettings fromJSON(const nlohmann::json& j)
      {
        StatelineSettings s;
        s.ndims = readSettings<uint>(j, "dimensionality");
        s.nstacks = readSettings<uint>(j, "parallelTempering", "stacks");
        s.nchains = readSettings<uint>(j, "parallelTempering", "chains");
        s.annealLength = readSettings<uint>(j, "annealLength");
        s.nsamples = readSettings<uint>(j, "nsamples");
        s.swapInterval = readSettings<uint>(j,"parallelTempering","swapInterval");
        s.msLoggingRefresh = readSettings<int>(j, "msLoggingRefresh");
        s.sigmaSettings = mcmc::SlidingWindowSigmaSettings::fromJSON(j);
        s.betaSettings = mcmc::SlidingWindowBetaSettings::fromJSON(j);
        s.chainSettings.databasePath = readSettings<std::string>(j, "output", "directory");
        s.chainSettings.chainCacheLength = readSettings<uint>(j,"output", "cacheLength");
        s.nJobTypes = readSettings<uint>(j, "nJobTypes");

        if (j.count("boundaries"))
          s.proposalBounds = mcmc::ProposalBounds::fromJSON(j);

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
      uint port_;
      StatelineSettings settings_;
      bool running_;
      zmq::context_t* context_;
      std::future<void> serverThread_;
      std::future<void> samplerThread_;
      std::future<void> apiServerThread_;
  };
}
