//!
//! A delegator that runs parallel tempering on several workers to perform an inversion.
//!
//! \file obsidian.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \author Nahid Akbar
//! \date February, 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

// Standard Library
#include <random>
#include <thread>
#include <chrono>
#include <numeric>
// Prerequisites
#include <glog/logging.h>
#include <boost/program_options.hpp>
// Project
#include "app/console.hpp"
#include "app/settings.hpp"
#include "input/input.hpp"
#include "comms/delegator.hpp"
#include "comms/requester.hpp"
#include "serial/serial.hpp"
#include "app/asyncdelegator.hpp"
#include "app/signal.hpp"
#include "infer/mcmc.hpp"
#include "infer/metropolis.hpp"
#include "infer/adaptive.hpp"
#include "fwdmodel/fwd.hpp"
#include "datatype/sensors.hpp"
#include "detail.hpp"

using namespace obsidian;
using namespace stateline;

namespace ph = std::placeholders;

// Command line options specific to obsidian
po::options_description commandLineOptions()
{
  po::options_description cmdLine("Delegator Command Line Options");
  cmdLine.add_options() //
  ("port,p", po::value<uint>()->default_value(5555), "TCP port to accept connections") //
  ("configfile,c", po::value<std::string>()->default_value("obsidian_config"), "configuration file") //
  ("inputfile,i", po::value<std::string>()->default_value("input.obsidian"), "input file") //
  ("recover,r", po::bool_switch()->default_value(false), "force recovery")
  ("anneallength,a", po::value<uint>()->default_value(1000), "anneal chains with n samples before starting mcmc");
  return cmdLine;
}

int main(int ac, char* av[])
{
  init::initialiseSignalHandler();
  // Get the settings from the command line
  auto vm = init::initProgramOptions(ac, av, commandLineOptions());

  // Set up a random generator here
  std::random_device rd;
  std::mt19937 gen(rd());

  LOG(INFO)<< "Initialise the various settings and objects";

  readConfigFile(vm["configfile"].as<std::string>(), vm);
  readInputFile(vm["inputfile"].as<std::string>(), vm);

  std::set<ForwardModel> sensorsEnabled = parseSensorsEnabled(vm);
  DelegatorSettings delegatorSettings = parseDelegatorSettings(vm);
  MCMCSettings mcmcSettings = parseMCMCSettings(vm);
  DBSettings dbSettings = parseDBSettings(vm);
  dbSettings.recover = vm["recover"].as<bool>();
  WorldSpec worldSpec = parseSpec<WorldSpec>(vm, sensorsEnabled);
  GlobalPrior prior = parsePrior<GlobalPrior>(vm, sensorsEnabled);
  GlobalResults results = loadResults(worldSpec, vm, sensorsEnabled);

  LOG(INFO)<< "Create the specification data (serialised) for all the jobs";
  std::string worldSpecData = obsidian::comms::serialise(worldSpec);
  std::vector<uint> sensorId;
  std::vector<std::string> sensorSpecData;
  std::vector<std::string> sensorReadings;
  applyToSensorsEnabled<getData>(sensorsEnabled, std::ref(sensorId), std::ref(sensorSpecData), std::ref(sensorReadings),
                                 std::cref(worldSpec), std::ref(results), std::cref(vm), std::cref(sensorsEnabled));

  LOG(INFO)<< "Initialise comms";
  stateline::comms::Delegator delegator(worldSpecData, sensorId, sensorSpecData, sensorReadings, delegatorSettings);
  delegator.start();

  LOG(INFO)<< "Initialise parallel tempering mcmc";
  GeoAsyncPolicy policy(delegator, prior, sensorsEnabled);

  // Start the sampling
  LOG(INFO)<< "Starting inversion: Run for " << mcmcSettings.wallTime << " seconds";
  LOG(INFO)<< "Problem dimensionality: " << prior.size();
  mcmc::Sampler mcmc(mcmcSettings, dbSettings, prior.size(), global::interruptedBySignal);

  std::vector<Eigen::VectorXd> initialThetas;

  LOG(INFO)<< "Loading / generating initial thetas for the chains";

  for (uint s = 0; s < mcmcSettings.stacks; s++)
  {
    for (uint c = 0; c < mcmcSettings.chains; c++)
    {
      if (dbSettings.recover && !mcmc.chains().states(s * mcmcSettings.chains + c).empty())
      {
        initialThetas.push_back(mcmc.chains().states(s * mcmcSettings.chains + c)[0].sample);
      } else
      {
        uint nSamples = vm["anneallength"].as<uint>();
        std::vector<Eigen::VectorXd> thetas(nSamples);
        double lowestEnergy = std::numeric_limits<double>::max();
        uint bestIndex = 0;
        for ( uint i=0; i<nSamples; i++)
        {
          Eigen::VectorXd cand = prior.sample(gen);
          thetas[i] = cand;
          policy.submit(i,cand);
        }
        for (uint i=0; i<nSamples; i++)
        {
          auto result = policy.retrieve();
          uint id = result.first;
          double energy = result.second;
          if (energy < lowestEnergy)
          {
            bestIndex = id;
            lowestEnergy = energy;
            LOG(INFO) << "stack "<< s << " chain " << c << " best energy: " << lowestEnergy;
          }
        }
        initialThetas.push_back(thetas[bestIndex]);
      }
    }
  }

  
  auto proposal = std::bind(&mcmc::adaptiveGaussianProposal,ph::_1, ph::_2,
                            prior.world.thetaMinBound(), prior.world.thetaMaxBound()); 
  mcmc.run(policy, initialThetas, proposal, mcmcSettings.wallTime);

  // This will gracefully stop all delegators internal threads
  delegator.stop();

  return 0;
}
