//!
//! Contains the implementation of Monte Carlo Markov Chain simulations.
//!
//! \file infer/mcmc.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <limits>
#include <random>
#include <chrono>
#include <map>
#include <Eigen/Dense>
#include <iostream>
#include <glog/logging.h>

#include "db/db.hpp"
#include "comms/datatypes.hpp"
#include "comms/settings.hpp"
#include "comms/serial.hpp"
#include "comms/delegator.hpp"
#include "comms/requester.hpp"
#include "infer/async.hpp"
#include "infer/settings.hpp"
#include "infer/chainarray.hpp"
#include "infer/diagnostics.hpp"
#include "app/serial.hpp"

namespace stateline
{
  namespace mcmc
  {
    using JobConstructFunction =
      std::function<std::vector<comms::JobData>(const Eigen::VectorXd&)>;

    using ResultEnergyFunction = 
      std::function<double(const std::vector<comms::ResultData>&)>;

    using ProposalFunction = std::function<Eigen::VectorXd(uint id, const mcmc::ChainArray& chains)>;


    class WorkerInterface
    {
      public:
        WorkerInterface(const std::string& globalSpecData,
            const std::map<comms::JobID, std::string>& perJobSpecData,
            const JobConstructFunction& jobConstructFn,
            const ResultEnergyFunction& resultEnergyFn,
            const DelegatorSettings& settings)

          : jobConstructFn_(jobConstructFn),
            resultEnergyFn_(resultEnergyFn),
            delegator_(globalSpecData, perJobSpecData, settings),
            requester_(delegator_)
        {
          delegator_.start();
        }

        void submit(uint id, const Eigen::VectorXd& x)
        {
          requester_.batchSubmit(id, jobConstructFn_(x));
        }

        std::pair<uint, double> retrieve()
        {
          auto result = requester_.batchRetrieve();
          return std::make_pair(result.first, resultEnergyFn_(result.second));
        }

      private:
        JobConstructFunction jobConstructFn_;
        ResultEnergyFunction resultEnergyFn_;
        comms::Delegator delegator_;
        comms::Requester requester_;
    };

    std::vector<comms::JobData> singleJobConstruct(const Eigen::VectorXd &x);
    double singleJobLikelihood(const std::vector<comms::ResultData> &results);

  } // namespace mcmc 
} // namespace stateline
