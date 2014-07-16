//!
//! Contains commonly used async policies and delegator helper functions.
//!
//! \file app/delegator.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <Eigen/Dense>

#include "app/serial.hpp"
#include "comms/delegator.hpp"
#include "comms/requester.hpp"

namespace stateline
{
  class SingleTaskAsyncPolicy
  {
    public:
      SingleTaskAsyncPolicy(comms::Delegator &delegator)
        : req_(delegator)
      {
      }

      void submit(uint id, const Eigen::VectorXd &x)
      {
        // Serialise the vector into a string
        req_.submit(id, { 0, "", serialise(x) });
      }

      std::pair<uint, double> retrieve()
      {
        // Unserialise the job result back into a likelihood
        auto msg = req_.retrieve();
        return std::make_pair(msg.first, *((double *)msg.second.data.data()));
      }

    private:
      comms::Requester req_;
  };

  using SplitFuncType =
    std::function<std::vector<comms::JobData>(const Eigen::VectorXd &)>;

  using CombineFuncType = 
    std::function<double(const std::vector<comms::ResultData>)>;

  std::vector<comms::JobData> duplicateSplitFunc(const std::vector<comms::JobID> jobs,
      const Eigen::VectorXd &x)
  {
    std::vector<comms::JobData> result;
    for (auto job : jobs)
    {
      result.push_back({ job, "", serialise(x) });
    }
    return result;
  }

  double sumCombineFunc(const std::vector<comms::ResultData> &results)
  {
    double logl = 0;
    for (const comms::ResultData &result : results)
    {
      logl += *((double *)result.data.data());
    }
    return logl;
  }

  template <class SplitFunc = SplitFuncType, class CombineFunc = CombineFuncType>
  class MultiTaskAsyncPolicy
  {
    public:
      MultiTaskAsyncPolicy(comms::Delegator &delegator)
        : MultiTaskAsyncPolicy(delegator, duplicateSplitFunc, sumCombineFunc)
      {
      }

      MultiTaskAsyncPolicy(comms::Delegator &delegator,
          const SplitFunc &splitFunc, const CombineFunc &combineFunc)
        : req_(delegator), splitFunc_(splitFunc), combineFunc_(combineFunc),
          jobs_(delegator.jobs())
      {
      }

      void submit(uint id, const Eigen::VectorXd &x)
      {
        req_.batchSubmit(id, splitFunc_(x));
      }

      std::pair<uint, double> retrieve()
      {
        auto msg = req_.batchRetrieve();
        return std::make_pair(msg.first, combineFunc_(msg.second));
      }

    private:
      comms::Requester req_;
      SplitFunc splitFunc_;
      CombineFunc combineFunc_;
      std::vector<comms::JobID> jobs_;
  };
}
