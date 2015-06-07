//!
//! Stateline worker interface.
//!
//! \file worker.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2015, NICTA
//!

#include "worker.hpp"

#include <future>
#include <zmq.hpp>

#include "comms/minion.hpp"
#include "comms/worker.hpp"

namespace stateline
{

namespace
{

void runClient(std::string addr, zmq::context_t& ctx, bool& running)
{
  auto settings = comms::WorkerSettings::Default(addr);
  comms::Worker worker(ctx, settings, running);
  worker.start(); // Blocking call
}

void runWorker(LogLFn logLFn, const std::vector<std::string>& jobTypes,
    zmq::context_t& ctx, bool& running)
{
  comms::Minion minion{ctx, jobTypes};
  while (running)
  {
    auto job = minion.nextJob();
    minion.submitResult(logLFn(job.first, job.second));
  }
}

} // namespace

void runWorkers(const LogLFn& logLFn, const std::string& addr,
    const std::vector<std::string>& jobTypes,
    uint nThreads)
{
  zmq::context_t* ctx = new zmq::context_t{1};
  bool running = true;

  // Launch the client thread
  auto clientThread = std::async(std::launch::async, runClient, addr,
      std::ref(*ctx), std::ref(running));

  // Launch the worker threads
  std::vector<std::future<void>> workerThreads;
  for (uint i = 0; i < nThreads; i++)
  {
    workerThreads.push_back(
        std::async(std::launch::async, runWorker, logLFn, std::cref(jobTypes),
          std::ref(*ctx), std::ref(running)));
  }
}

} // namespace stateline
