//!
//! Contains commonly used async policies.
//!
//! \file app/async.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "comms/delegator.hpp"
#include "comms/requester.hpp"

namespace stateline
{
  template <class SplitFunc, class CombineFunc>
  class DelegatorAsyncPolicy
  {
    public:
      DelegatorAsyncPolicy(comms::Delegator delegator,
          const SplitFunc &splitFunc = SplitFunc(),
          const CombineFunc &combineFunc = CombineFunc())
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
  }
}
