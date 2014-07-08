//!
//! Contains commonly used async policies.
//!
//! \file app/async.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <Eigen/Dense>

#include "comms/delegator.hpp"
#include "comms/requester.hpp"

namespace stateline
{
  using SplitFuncType =
    std::function<std::vector<comms::JobData>(const Eigen::VectorXd &)>;

  using CombineFuncType = 
    std::function<double(const std::vector<comms::ResultData>)>;

  template <class SplitFunc = SplitFuncType, class CombineFunc = CombineFuncType>
  class DelegatorAsyncPolicy
  {
    public:
      DelegatorAsyncPolicy(comms::Delegator &delegator)
        : req_(delegator)
      {
      }

      DelegatorAsyncPolicy(comms::Delegator &delegator,
          const SplitFunc &splitFunc, const CombineFunc &combineFunc)
        : req_(delegator), splitFunc_(splitFunc), combineFunc_(combineFunc)
      {
      }

      void submit(uint id, const Eigen::VectorXd &theta)
      {
        req_.batchSubmit(id, splitFunc_(theta));
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
}
