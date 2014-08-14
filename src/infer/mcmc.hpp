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

    using ProposalFunction = std::function<Eigen::VectorXd(uint id, const mcmc::ChainArray&)>;

    struct ProblemInstance
    {
      std::string globalJobSpecData;
      std::map<comms::JobID, std::string> perJobSpecData;

      JobConstructFunction jobConstructFn;
      ResultEnergyFunction resultLikelihoodFn;
      ProposalFunction proposalFn;
    };

    struct SamplerSettings
    {
      MCMCSettings mcmc;
      DBSettings db;
      DelegatorSettings del;
    };

    std::vector<comms::JobData> singleJobConstruct(const Eigen::VectorXd &x)
    {
      return {{ 0, "", serialise(x) }};
    }

    double singleJobLikelihood(const std::vector<comms::ResultData> &results)
    {
      return comms::detail::unserialise<double>(results[0].data);
    }

  } // namespace mcmc 
} // namespace stateline
