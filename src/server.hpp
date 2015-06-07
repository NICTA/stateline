//!
//! Main entry point for using stateline -- server side
//!
//! \file app/server.hpp
//! \author Lachlan McCalman
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2015, NICTA
//!

//#pragma once

#include <future>

#include "infer/adaptive.hpp"
#include "infer/samplesarray.hpp"
#include "infer/sampler.hpp"

namespace stateline
{
  struct StatelineSettings
  {
      uint ndims;
      uint nstacks;
      uint nchains;
      uint swapInterval;
      int msLoggingRefresh;
      mcmc::SlidingWindowSigmaSettings sigmaSettings;
      mcmc::SlidingWindowBetaSettings betaSettings;
      std::vector<std::string> jobTypes;

      // TODO: add json without needing a #include <json>
      static StatelineSettings fromDefault(uint numDims,
          const std::vector<std::string>& jobTypes,
          uint numStacks = 1, uint numChains = 1);
  };

  class Server
  {
    public:
      Server(uint port, const StatelineSettings& s);
      ~Server();
      mcmc::SamplesArray step(uint length);
      void start();
      void stop();
      bool isRunning();

    private:
      mcmc::SamplesArray runSampler(uint length);

      struct Impl;
      std::unique_ptr<Impl> impl_;

      uint port_;
      StatelineSettings settings_;
      bool running_;

      std::future<void> serverThread_;
  };
}
