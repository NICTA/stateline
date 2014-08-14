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

    //! The number of samples from the beginning 
    uint adaptionLength;

    //! The number of samples kept in memory. The MCMC uses a sliding buffer
    //! which flushes to disk old samples when the cache is full.
    uint cacheLength;

    //! The initial proposal width factor. Initially, the proposal width of
    //! a chain will be initialSigmaFactor times the proposal width of the
    //! chain below (colder) than it.
    double initialSigmaFactor;
 
    //! The initial proposal width of the coldest chains.
    double proposalInitialSigma;

    //! The minimum proposal adaption factor allowed. This controls how much
    //! the proposal width can change in one adaption step.
    double proposalMinFactor;

    //! The maximum proposal adaption factor allowed. This controls how much
    //! the proposal width can change in one adaption step.
    double proposalMaxFactor;

    //! The optimal acceptance rate for any of the chains.
    double proposalOptimalAccept;

    //! The rate at which the proposal width changes when it adapts.
    double proposalAdaptRate;

    //! The number of samples between each attempt to adapt the proposal width.
    uint proposalAdaptInterval;

    //! The optimal swap rate for any of the chains.
    double betaOptimalSwapRate;

    //! The rate at which the inverse temperature changes when it adapts.
    double betaAdaptRate;

    //! The minimum beta adaption factor allowed. This controls how much
    //! the temperature can change in one adaption step.
    double betaMinFactor;

    //! The minimum beta adaption factor allowed. This controls how much
    //! the temperature can change in one adaption step.
    double betaMaxFactor;

    //! The number of samples between each attempt to adapt the temperature.
    uint betaAdaptInterval;

    //! The initial temperature ladder factor.
    double initialTempFactor;

    //! Empty settings with all parameters set to zero.
    static MCMCSettings Empty()
    {
      MCMCSettings settings = {};
      return settings;
    }

    static MCMCSettings NoAdaption(uint stacks, uint chains)
    {
      MCMCSettings settings = MCMCSettings::Empty();
      settings.stacks = stacks;
      settings.chains = chains;
      settings.swapInterval = 25;
      settings.adaptionLength = 1;
      settings.cacheLength = 100;
      settings.initialSigmaFactor = 1.1;
      settings.proposalInitialSigma = 1.0;
      settings.proposalMinFactor = 1.0;
      settings.proposalMaxFactor = 1.0;
      settings.proposalOptimalAccept = 0.4;
      settings.proposalAdaptInterval = 100000000;
      settings.betaOptimalSwapRate = 0.5;
      settings.betaAdaptRate = 1.0;
      settings.betaMinFactor = 1.0;
      settings.betaMaxFactor = 1.0;
      settings.betaAdaptInterval = 1000000000;
      settings.initialTempFactor = 1.2;
      return settings;
    }

    static MCMCSettings Default(uint chains, uint stacks)
    {
      MCMCSettings settings = MCMCSettings::Empty();
      settings.chains = chains;
      settings.stacks = stacks;
      settings.swapInterval = 25;
      settings.adaptionLength = 2900;
      settings.cacheLength = 100;
      settings.initialSigmaFactor = 1.1;
      settings.proposalInitialSigma = 1.0;
      settings.proposalMinFactor = 0.80;
      settings.proposalMaxFactor = 1.25;
      settings.proposalOptimalAccept = 0.18;
      settings.proposalAdaptRate = 0.20;
      settings.proposalAdaptInterval = 100;
      settings.betaOptimalSwapRate = 0.24;
      settings.betaAdaptRate = 0.04;
      settings.betaMinFactor = 1.0;
      settings.betaMaxFactor = 1.0;
      settings.betaAdaptInterval = 250;
      settings.initialTempFactor = 1.5;
      return settings;
    }
  };
}
