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
#include <random>

namespace stateline
{

void runMinion(const LikelihoodFn& lhFn, const std::pair<uint, uint>& jobTypesRange,
               zmq::context_t& context, const std::string& workerSocketAddr, bool& running)
{
  comms::Minion minion(context, jobTypesRange, workerSocketAddr);
  while (running)
  {
    const auto job = minion.nextJob();
    const auto& jobType = job.first;
    const auto& sample = job.second;
    double nll = lhFn(jobType,sample);
    minion.submitResult(nll);
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

WorkerWrapper::WorkerWrapper(const LikelihoodFn& f, const std::pair<uint, uint>& jobTypesRange,
    const std::string& address)
  : lhFn_(f)
  , jobTypesRange_(jobTypesRange)
  , settings_(comms::WorkerSettings::Default(address, generateRandomIPCAddr()))
{
}


void WorkerWrapper::start()
{
  context_ = new zmq::context_t{1};
  running_ = true;

  clientThread_ = startInThread<comms::Worker>(std::ref(running_), std::ref(*context_),
                                               std::cref(settings_));

  minionThread_ = std::async(std::launch::async, runMinion, std::cref(lhFn_),
                             std::cref(jobTypesRange_), std::ref(*context_),
                             std::cref(settings_.workerAddress), std::ref(running_));
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
