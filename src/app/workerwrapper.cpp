//!
//! Main entry point for using stateline -- worker side
//!
//! 
//!
//! \file app/workerwrapper.cpp
//! \author Lachlan McCalman
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2015, NICTA
//!

#include "workerwrapper.hpp"
#include "../comms/minion.hpp"
#include "../comms/worker.hpp"

namespace stateline
{

void runMinion(const LikelihoodFn& f, zmq::context_t& context, const std::vector<std::string>& jobTypes, bool& running)
{
  comms::Minion minion(context, jobTypes);
  while (running)
  {
    auto job = minion.nextJob();
    auto jobType = job.first;
    auto sample = job.second;
    minion.submitResult(f(jobType, sample));
  }
}

void runClient(zmq::context_t& context, const std::string& address, bool& running)
{
  auto settings = comms::WorkerSettings::Default(address);
  comms::Worker worker(context, settings, running);
  worker.start();
}


WorkerWrapper::WorkerWrapper(const LikelihoodFn& f, const std::string& address, const std::vector<std::string>& jobTypes, uint nThreads)
  : f_(f), address_(address), jobTypes_(jobTypes), nThreads_(nThreads) 
{
}

void WorkerWrapper::start()
{
  context_ = new zmq::context_t{1};
  running_ = true;
  clientThread_ = std::async(std::launch::async, runClient, std::ref(*context_), std::cref(address_), std::ref(running_));
  for (uint i=0; i<nThreads_;i++)
  {
    wthreads_.push_back(
        std::async(std::launch::async, runMinion, std::cref(f_), std::ref(*context_), std::cref(jobTypes_), std::ref(running_))
        );
  }
}

void WorkerWrapper::stop()
{
  running_ = false;
  if (context_)
  {
    delete context_;
    context_ = nullptr; //THIS MUST BE DONE
  }
  clientThread_.wait();
  for (auto const& t : wthreads_)
  {
    t.wait(); 
  }
}

WorkerWrapper::~WorkerWrapper()
{
  stop();
}


}
