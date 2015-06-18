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
#include "comms/minion.hpp"
#include "comms/worker.hpp"
#include "comms/thread.hpp"

#include <iomanip>

namespace stateline
{

void runMinion(const JobLikelihoodFnMap& m, zmq::context_t& context,
               const std::string& workerSocketAddr, bool& running )
{
  // Create vector of job types from JobLikelihoodFnMap keys
  std::vector<std::string> jobsVec;
  std::transform( m.begin(), m.end(), std::back_inserter(jobsVec),
                  [](const JobLikelihoodFnMap::value_type &v){ return v.first; } );

  comms::Minion minion(context, jobsVec, workerSocketAddr);
  while (running)
  {
    const auto job = minion.nextJob();
    const auto& jobType = job.first;
    const auto& sample = job.second;
    minion.submitResult( m.at(jobType)(jobType,sample) );
  }
}

std::string generateRandomIPCAddr()
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 0x1000000);
  std::ostringstream oss;
  oss << "ipc:///tmp/sl_worker_"
     << std::hex << std::uppercase << std::setw(6) << std::setfill('0') << dis(gen)
     << ".socket";
  return oss.str();
}

WorkerWrapper::WorkerWrapper(const JobLikelihoodFnMap& m, const std::string& address)
  : m_(m), settings_(comms::WorkerSettings::Default(address))
{
  settings_.workerAddress = generateRandomIPCAddr();
}

void WorkerWrapper::start()
{
  context_ = new zmq::context_t{1};
  running_ = true;

  clientThread_ = startInThread<comms::Worker>(std::ref(running_), std::ref(*context_),
                                               std::cref(settings_));

  minionThread_ = std::async(std::launch::async, runMinion, std::cref(m_),
                             std::ref(*context_), std::cref(settings_.workerAddress),
                             std::ref(running_));
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
