//!
//! Settings for MCMC.
//!
//! \file infer/settings.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

namespace stateline
{
  //! Settings for Markov Chain Monte Carlo simulations.
  //!
  struct MCMCSettings
  {
    //! The number of stacks used in the simulation.
    uint stacks;
    
    //! The number of chains per stack.
    uint chains;

    //! The number of samples between swap attempts.
    uint swapInterval;

    //! The number of samples kept in memory. The MCMC uses a sliding buffer
    //! which flushes to disk old samples when the cache is full.
    uint cacheLength;

    //! Empty settings with all parameters set to zero.
    static MCMCSettings Empty()
    {
      MCMCSettings settings;
      return settings;
    }

    static MCMCSettings Default(uint stacks, uint chains)
    {
      MCMCSettings settings = MCMCSettings::Empty();
      settings.stacks = stacks;
      settings.chains = chains;
      settings.swapInterval = 25;
      settings.cacheLength = 1000;
      return settings;
    }
  };

  struct SamplerSettings
  {
    MCMCSettings mcmc;
    DBSettings db;
    DelegatorSettings del;
  };



}
