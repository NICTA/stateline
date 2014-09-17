//!
//! Contains the interface to the worker as seen by the delegator
//!
//! \file comms/workerinterface.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <functional>
#include <vector>
#include <map>
#include <glog/logging.h>
#include "comms/datatypes.hpp"
#include "comms/settings.hpp"
#include "comms/delegator.hpp"
#include "comms/requester.hpp"

namespace stateline
{
  namespace comms
  {
    template <typename JobInput, typename ResultOutput>
    class WorkerInterface
    {
      public:
        WorkerInterface(const std::string& globalSpecData,
            const std::map<JobID, std::string>& perJobSpecData,
            const std::function<std::vector<JobData>(JobInput)>& jobInputFn,
            const std::function<ResultOutput(const std::vector<ResultData>&)>& resultOutputFn,
            const DelegatorSettings& settings)

          : jobInputFn_(jobInputFn),
            resultOutputFn_(resultOutputFn)
        {
          delegator_ = new Delegator(globalSpecData, perJobSpecData, settings);
          requester_ = new Requester(*delegator_);
        }

        ~WorkerInterface()
        {
          VLOG(1) << "Worker interface deleting requester";
          delete requester_;
          VLOG(1) << "Requester deleted";
          VLOG(1) << "Worker interface deleting delegator";
          delete delegator_;
          VLOG(1) << "Delegator deleted";
        }

        void submit(uint id, const Eigen::VectorXd& x)
        {
          requester_->batchSubmit(id, jobInputFn_(x));
        }

        std::pair<uint, double> retrieve()
        {
          auto result = requester_->batchRetrieve();
          return std::make_pair(result.first, resultOutputFn_(result.second));
        }

      private:
        std::function<std::vector<JobData>(JobInput)> jobInputFn_;
        std::function<ResultOutput(const std::vector<ResultData>&)> resultOutputFn_;
        Delegator* delegator_;
        Requester* requester_;
    };
  }
}
