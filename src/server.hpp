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

#include "infer/adaptive.hpp"
#include "infer/chainarray.hpp"

namespace stateline
{
  class SamplesArray
  {
    private:

  };

  struct StatelineSettings
  {
      uint ndims;
      uint nstacks;
      uint nchains;
      uint nsecs;
      uint swapInterval;
      int msLoggingRefresh;
      mcmc::SlidingWindowSigmaSettings sigmaSettings;
      mcmc::SlidingWindowBetaSettings betaSettings;
      std::vector<std::string> jobTypes;

      static StatelineSettings fromJSON(const nlohmann::json& j);
  };

  class Server
  {

    public:
      Server(uint port, const StatelineSettings& s);
      ~Server();
      SamplesArray step(uint length);
      void stop();
      bool isRunning();

    private:
      // TODO: use PIMPL to hide these members
      uint port_;
      StatelineSettings settings_;
      bool running_;
      zmq::context_t* context_;
      std::future<void> serverThread_;
      std::future<void> samplerThread_;
  };
}
