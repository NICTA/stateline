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

  template <class SplitFunc = SplitFuncType, class CombineFunc = CombineFuncType>
  class MultiTaskAsyncPolicy
  {
    public:
      MultiTaskAsyncPolicy(comms::Delegator &delegator)
        : req_(delegator)
      {
      }

      MultiTaskAsyncPolicy(comms::Delegator &delegator,
          const SplitFunc &splitFunc, const CombineFunc &combineFunc)
        : req_(delegator), splitFunc_(splitFunc), combineFunc_(combineFunc)
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
  };

  template <class AsyncPolicy, class Func>
  auto runDelegatorWithPolicy(const Func &func, const DelegatorSettings &settings) ->
    decltype(func(std::declval<AsyncPolicy &>()))
  {
    // Start a delegator with no global specification and single job type
    comms::Delegator delegator("", {{ 0, "" }}, settings);
    delegator.start();

    // Initialise the policy and run the user-specified function
    AsyncPolicy policy(delegator);
    return func(policy);
  }

  template <class AsyncPolicy, class Func>
  auto runDelegatorWithPolicy(const Func &func, const std::string &spec, const DelegatorSettings &settings) ->
    decltype(func(std::declval<AsyncPolicy &>(), spec))
  {
    // Start a delegator with no global specification and single job type
    comms::Delegator delegator(spec, {{ 0, "" }}, settings);
    delegator.start();

    // Initialise the policy and run the user-specified function
    AsyncPolicy policy(delegator);
    return func(policy, spec);
  }
}
