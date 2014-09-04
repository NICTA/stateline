//!
//! Contains the MCMC datatypes
//!
//! \file infer/datatypes.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <Eigen/Dense>
#include <glog/logging.h>
#include "comms/workerinterface.hpp"


namespace stateline
{
  namespace mcmc
  {

    //! Used for recording swapping of chains.
    //!
    enum class SwapType
    {
      //! No swap was attempted.
      NoAttempt,

      //! A swap was accepted.
      Accept,

      //! A swap was rejected.
      Reject
    };

    //! Used to represent a state in a Markov Chain.
    //!
    struct State
    {
      //! The actual sample that the state represents.
      Eigen::VectorXd sample;

      //! The energy (negative log likelihood) of the state.
      double energy;

      //! The step size of the chain when this state was recorded.
      Eigen::VectorXd sigma;

      //! The inverse temperature of the chain when this state was recorded.
      double beta;

      //! Whether this state was an accepted proposal state or a copy of the previous state.
      bool accepted;

      //! The type of swap that occurred when this state was recorded.
      SwapType swapType;

    };

    using WorkerInterface = 
      comms::WorkerInterface<const Eigen::VectorXd&, double>;

    std::vector<comms::JobData> singleJobConstruct(const Eigen::VectorXd &x);
    double singleJobEnergy(const std::vector<comms::ResultData> &results);

  } // namespace mcmc 
} // namespace stateline
