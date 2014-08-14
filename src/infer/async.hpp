//!
//! Contains asynchronous communicator.
//!
//! \file infer/async.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "comms/datatypes.hpp"
#include "comms/delegator.hpp"
#include "comms/requester.hpp"

namespace stateline
{
  namespace mcmc
  {
    class AsyncCommunicator
    {
      public:
        AsyncCommunicator(const std::string& commonSpecData,
              const std::map<comms::JobID, std::string>& jobSpecData,
              const DelegatorSettings& settings)
            : delegator_(commonSpecData, jobSpecData, settings),
              requester_(delegator_)
        {
          delegator_.start();
        }

        void submit(uint id, const std::vector<comms::JobData>& jobs)
        {
          requester_.batchSubmit(id, jobs);
        }
      
        std::pair<uint, std::vector<comms::ResultData>> retrieve()
        {
          return requester_.batchRetrieve();
        }

      private:
        comms::Delegator delegator_;
        comms::Requester requester_;
    };
  }
}
