//!
//! Main entry point for using stateline -- worker side
//!
//! 
//!
//! \file app/workerwrapper.hpp
//! \author Lachlan McCalman
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2015, NICTA
//!

#include "../comms/settings.hpp"

#include <map>
#include <future>
#include <zmq.hpp>

namespace stateline
{
  typedef std::function<double(uint, const std::vector<double>&)> LikelihoodFn;

  class WorkerWrapper
  {
    public:
      WorkerWrapper(const LikelihoodFn& f, const std::pair<uint, uint>& jobTypesRange,
                    const std::string& address);

      ~WorkerWrapper();
      void start();
      void stop();

    private:

      const LikelihoodFn lhFn_;
      std::pair<uint, uint> jobTypesRange_;

      comms::WorkerSettings settings_;

      bool running_;
      zmq::context_t* context_;

      std::future<bool> clientThread_;
      std::future<void> minionThread_;
  };
}

