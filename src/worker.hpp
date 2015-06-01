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

#include <future>
#include <zmq.hpp>

namespace stateline
{
  typedef std::function<double(const std::string&, const std::vector<double>&)> LikelihoodFn;

  class Worker
  {
    public:
      Worker(const LikelihoodFn& f, const std::string& address, const std::vector<std::string>& jobTypes, uint nThreads);
      ~Worker();
      void start();
      void stop();

    private:
      const LikelihoodFn& f_;
      std::string address_;
      std::vector<std::string> jobTypes_;
      uint nThreads_;
      bool running_;
      zmq::context_t* context_;
      std::future<void> clientThread_;
      std::vector<std::future<void>> wthreads_;
  };
}
