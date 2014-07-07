//!
//! Input implementations related to mcmc.
//!
//! \file mcmc.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "common.hpp"

namespace obsidian
{
  void initMCMCOptions(po::options_description & options)
  {
    options.add_options()("mcmc.chains", po::value<uint>(), "number of chains per stack")("mcmc.stacks", po::value<uint>(),
                                                                                          "number of stacks")(
        "mcmc.wallTime", po::value<uint>(), "number of seconds to run (approximately)")("mcmc.swapInterval", po::value<uint>(),
                                                                                        "steps before PT swap attempted")(
        "mcmc.initialTempFactor", po::value<double>(), "geometric multiplier for temperature sequence")("mcmc.betaOptimalSwapRate",
                                                                                                        po::value<double>(), "")(
        "mcmc.betaAdaptRate", po::value<double>(), "adaption rate for beta")("mcmc.betaMinFactor", po::value<double>(),
                                                                             "minimum beta adaption factor")("mcmc.betaMaxFactor",
                                                                                                             po::value<double>(),
                                                                                                             "maximum beta adaption factor")(
        "mcmc.betaAdaptInterval", po::value<uint>(), "interval over which beta is adapted")("mcmc.adaptionLength", po::value<uint>(),
                                                                                            "Total chain length before adaption stops")(
        "mcmc.cacheLength", po::value<uint>(), "Total chain length before adaption stops")("proposal.initialSigma", po::value<double>(),
                                                                                           "initial proposal standard deviation")(
        "proposal.initialSigmaFactor", po::value<double>(), "initial proposal standard deviation")("proposal.maxFactor",
                                                                                                   po::value<double>(),
                                                                                                   "maximum adaption factor")(
        "proposal.minFactor", po::value<double>(), "minimum adaption factor")("proposal.optimalAccept", po::value<double>(),
                                                                              "optimal acceptance ratio")(
        "proposal.adaptRate", po::value<double>(), "controls the amount by which the proposal width changes")(
        "proposal.adaptInterval", po::value<uint>(), "steps before proposal function re-adapts");
  }

  stateline::MCMCSettings parseMCMCSettings(const po::variables_map& vm)
  {
    LOG(INFO)<<"Parsing MCMC settings";
  stateline::MCMCSettings s;
  s.chains = vm["mcmc.chains"].as<uint>();
  s.stacks = vm["mcmc.stacks"].as<uint>();
  s.wallTime = vm["mcmc.wallTime"].as<uint>();
  s.swapInterval = vm["mcmc.swapInterval"].as<uint>();
  s.initialTempFactor = vm["mcmc.initialTempFactor"].as<double>();
  s.proposalInitialSigma = vm["proposal.initialSigma"].as<double>();
  s.initialSigmaFactor = vm["proposal.initialSigmaFactor"].as<double>();
  s.proposalMaxFactor = vm["proposal.maxFactor"].as<double>();
  s.proposalMinFactor = vm["proposal.minFactor"].as<double>();
  s.proposalOptimalAccept = vm["proposal.optimalAccept"].as<double>();
  s.proposalAdaptRate = vm["proposal.adaptRate"].as<double>();
  s.proposalAdaptInterval = vm["proposal.adaptInterval"].as<uint>();
  s.betaOptimalSwapRate = vm["mcmc.betaOptimalSwapRate"].as<double>();
  s.betaAdaptRate = vm["mcmc.betaAdaptRate"].as<double>();
  s.betaMinFactor = vm["mcmc.betaMinFactor"].as<double>();
  s.betaMaxFactor = vm["mcmc.betaMaxFactor"].as<double>();
  s.betaAdaptInterval = vm["mcmc.betaAdaptInterval"].as<uint>();
  s.adaptionLength = vm["mcmc.adaptionLength"].as<uint>();
  s.cacheLength = vm["mcmc.cacheLength"].as<uint>();
  return s;
}
}
