//!
//! Contains useful worker helper functions used to make worker apps.
//!
//! \file app/worker.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>

#include "comms/worker.hpp"
#include "comms/minion.hpp"

namespace stateline
{
  template <class WorkerFunc>
  void runMinion(comms::Worker &worker, uint jobType, const WorkerFunc &func)
  {
    comms::Minion minion(worker, jobType);
    while (true) {
      comms::JobData job = minion.nextJob();
      minion.submitResult({ job.type, func(job.type, job.globalData, job.jobData) });
    }
  }

  template <class WorkerFunc>
  std::future<void> runMinionThreaded(comms::Worker &worker, uint jobType, const WorkerFunc &func)
  {
    return std::async(std::launch::async, runMinion<WorkerFunc>, std::ref(worker), jobType, std::cref(func));
  }
}
