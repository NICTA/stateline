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
  typedef std::function<double(const std::string&, const std::vector<double>&)> LikelihoodFn;

  typedef std::function<const LikelihoodFn&(const std::string&)> JobToLikelihoodFnFn;

  typedef std::map<std::string, LikelihoodFn> JobLikelihoodFnMap;

  class WorkerWrapper
  {
    public:
      WorkerWrapper(const LikelihoodFn& f, const std::vector<std::string>& jobTypes,
                    const std::string& address);

      WorkerWrapper(const JobLikelihoodFnMap& m, const std::string& address);

      WorkerWrapper(const JobToLikelihoodFnFn& f, const std::vector<std::string>& jobTypes,
                    const std::string& address);

      ~WorkerWrapper();
      void start();
      void stop();

    private:

      const JobToLikelihoodFnFn lhFnFn_;
      std::vector<std::string> jobTypes_;

      comms::WorkerSettings settings_;

      bool running_;
      zmq::context_t* context_;

      std::future<bool> clientThread_;
      std::future<void> minionThread_;
  };
}

