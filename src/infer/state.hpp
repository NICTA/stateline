//!
//! State class for MCMC.
//!
//! \file infer/state.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA

#pragma once

#include <Eigen/Core>

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

      State()
      {
      }

      State(const Eigen::VectorXd &sample)
        : sample(sample)
      {
      }

      State(const Eigen::VectorXd &sample, double energy,
          const Eigen::VectorXd &sigma, double beta)
        : sample(sample), energy(energy), sigma(sigma), beta(beta)
      {
      }
    };
  }
}
