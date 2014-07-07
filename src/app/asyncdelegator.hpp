//!
//! Async policy for job multiplexing on server side
//!
//! \file app/asyncdelegator.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <queue>
#include "comms/delegator.hpp"
#include "comms/requester.hpp"
#include "comms/datatypes.hpp"
#include "prior/prior.hpp"
#include "datatype/forwardmodels.hpp"

namespace obsidian
{

  //! Abstraction / Wrapper for the comms system. Used from mcmc.run() for submitting jobs and retrieves results.
  //!
  class GeoAsyncPolicy
  {
  public:
    GeoAsyncPolicy(stateline::comms::Delegator &delegator, const GlobalPrior& prior, const std::set<ForwardModel> &sensorsEnabled);

    //! Submit job for a parameter set for all sensors.
    //!
    //! \param id a job ID (0 - uint32_t::max
    //! \param theta parameters to compute likelihood of.
    //!
    void submit(uint id, const Eigen::VectorXd &theta);

    //! Retrieve a job likelihood; collated over all the sensors.
    //!
    //! \return pair<job ID , likelihood>
    //!
    std::pair<uint, double> retrieve();

  private:

    stateline::comms::Requester req_;
    GlobalPrior prior_;
    std::map<uint, double> priorValues_;
    std::vector<uint> thetaLengths_;
    std::set<ForwardModel> sensorsEnabled_;
    std::queue<uint> zeroSet_;
  };
}
