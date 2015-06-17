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

void runMinion(const JobLikelihoodFnMap& m, zmq::context_t& context, bool& running)
{
  // Create vector of job types from JobLikelihoodFnMap keys
  std::vector<std::string> jobsVec;
  std::transform( m.begin(), m.end(), std::back_inserter(jobsVec),
                  [](const JobLikelihoodFnMap::value_type &v){ return v.first; } );

  comms::Minion minion(context, jobsVec);
  while (running)
  {
    const auto job = minion.nextJob();
    const auto& jobType = job.first;
    const auto& sample = job.second;
    minion.submitResult( m.at(jobType)(jobType,sample) );
  }
}

void runClient(zmq::context_t& context, const std::string& address, bool& running)
{
  auto settings = comms::WorkerSettings::Default(address);
  comms::Worker worker(context, settings, running);
  worker.start();
}


WorkerWrapper::WorkerWrapper(const JobLikelihoodFnMap& m, const std::string& address)
  : m_(m), address_(address)
{
}

void WorkerWrapper::start()
{
  context_ = new zmq::context_t{1};
  running_ = true;
  clientThread_ = std::async(std::launch::async, runClient, std::ref(*context_),
                             std::cref(address_), std::ref(running_));
  minionThread_ = std::async(std::launch::async, runMinion, std::cref(m_),
                             std::ref(*context_), std::ref(running_));
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
  minionThread_.wait();
}

WorkerWrapper::~WorkerWrapper()
{
  stop();
}

}
