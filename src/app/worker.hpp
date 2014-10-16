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

#include <Eigen/Dense>

#include "app/serial.hpp"
#include "comms/worker.hpp"
#include "comms/minion.hpp"

namespace stateline
{
  namespace detail
  {
    template <class F>
    struct IsLikelihoodFunction
    {
      template <class T>
      static constexpr auto check(int) ->
        typename
          std::is_same<
            decltype(std::declval<T>()(std::declval<const Eigen::VectorXd &>())),
            double
          >::type;

      template <class T>
      static constexpr std::false_type check(...);

      using type = decltype(check<F>(0));
    };

    template <class WorkerFunc>
    void runMinion(std::false_type, comms::Worker &worker, const WorkerFunc &func,
        comms::JobType jobType)
    {
      comms::Minion minion(worker, jobType);
      while (true)
      {
        comms::JobData job = minion.nextJob();
        minion.submitResult({ job.type, func(job.type, job.globalData, job.jobData) });
      }
    }

    template <class Model>
    void runMinion(std::true_type, comms::Worker &worker, const Model &model, uint jobType)
    {
      comms::Minion minion(worker, jobType);
      while (true)
      {
        comms::JobData job = minion.nextJob();

        // Unserialise the vector from the job data
        auto x = unserialise<Eigen::VectorXd>(job.jobData);

        // Evaluate the vector, serialise it into a byte string and submit.
        double result = model(x);
        minion.submitResult({ jobType, std::string((char *)&result, sizeof(double)) });
      }
    }
  }

  template <class WorkerFunc, class JobType>
  void runMinion(comms::Worker &worker, const WorkerFunc &func, JobType jobType)
  {
    return detail::runMinion(typename detail::IsLikelihoodFunction<WorkerFunc>::type(),
        worker, func, static_cast<uint>(jobType));
  }

  template <class WorkerFunc, class JobType = unsigned int>
  std::future<void> runMinionThreaded(comms::Worker &worker, const WorkerFunc &func, JobType jobType = 0)
  {
    return std::async(std::launch::async,
        runMinion<WorkerFunc, JobType>, std::ref(worker), std::cref(func), std::cref(jobType));
  }
}
