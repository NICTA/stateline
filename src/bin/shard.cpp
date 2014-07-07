//!
//! A worker running geophysical forward models.
//!
//! \file shard.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \author Nahid Akbar
//! \date February, 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

// Standard Library
#include <iostream>
#include <string>
#include <chrono>
#include <csignal>
#include <thread>

// Prerequisites
#include <glog/logging.h>
#include <boost/program_options.hpp>
#include <zmq.hpp>

// Project
#include "app/settings.hpp"
#include "input/input.hpp"
#include "app/console.hpp"
#include "io/npy.hpp"
#include "io/string.hpp"
#include "world/interpolate.hpp"
#include "world/transitions.hpp"
#include "world/voxelise.hpp"
#include "world/grid.hpp"
#include "comms/worker.hpp"
#include "comms/minion.hpp"
#include "serial/serial.hpp"
#include "likelihood/likelihood.hpp"
#include "fwdmodel/fwd.hpp"
#include "detail.hpp"
#include "app/signal.hpp"
#include "app/asyncworker.hpp"

// namespaces
using namespace obsidian;
using namespace stateline;

po::options_description commandLineOptions()
{
  std::string defaultJobListOptions = io::join(getAllJobTypes(), ",");

  po::options_description cmdLine("Delegator Command Line Options");
  cmdLine.add_options() //
  ("address,a", po::value<std::string>()->default_value("localhost:5555"), "Address and port of delegator") //
  ("jobtypes,j", po::value<std::string>()->default_value(defaultJobListOptions),
   ("subset of comma separated " + defaultJobListOptions).c_str()) //
  ("nthreads,t", po::value<uint>()->default_value(1), "Number of worker threads") //
  ("configfile,c", po::value<std::string>()->default_value("obsidian_config"), "configuration file");
  return cmdLine;
}

//! Each thread has a requester which deals with serialising and unserialising the jobs and results
//!
template<ForwardModel f>
bool workerModelThread(stateline::comms::Worker & worker, WorldSpec & worldSpec, std::vector<world::InterpolatorSpec> &interp,
                       const po::variables_map &vm)
{
  LOG(INFO)<< "Decoding " << f << " spec";
  typename Types<f>::Spec spec;
  obsidian::comms::unserialise(worker.jobSpec(static_cast<uint>(f)), spec);

  LOG(INFO) << "Generating " << f << " cache";
  typename Types<f>::Cache cache = fwd::generateCache<f>(interp, worldSpec, spec);

  LOG(INFO) << "Decoding " << f << " results";
  typename Types<f>::Results trueReadings;
  obsidian::comms::unserialise(worker.jobResults(static_cast<uint>(f)), trueReadings);

  uint nthreads = vm["nthreads"].as<uint>();
  LOG(INFO)<< "Launching " << nthreads << " worker threads for " << f;

  std::vector<std::future<bool>> threads;
  for (uint i = 0; i < nthreads; i++)
  {
    threads.push_back(
        std::async(std::launch::async, workerThread<f>, std::cref(spec), std::cref(cache), std::cref(trueReadings), std::ref(worker)));
  }
  for (auto& t : threads)
  {
    t.wait();
  }
  return true;
}

template<ForwardModel f>
struct launchWorkerThread
{
  launchWorkerThread(std::vector<std::future<bool>> & threads, stateline::comms::Worker & w, WorldSpec & ws, std::vector<world::InterpolatorSpec> &bi,
                     const po::variables_map &vm)
  {
    LOG(INFO)<< "Launching thread for " << f;
    threads.push_back(std::async(std::launch::async, workerModelThread<f>, std::ref(w), std::ref(ws), std::ref(bi), std::cref(vm)));
  }
};

int main(int ac, char* av[])
{
  // Get the settings from the command line
  auto vm = init::initProgramOptions(ac, av, commandLineOptions());

  LOG(INFO)<< "Reading config " << vm["configfile"].as<std::string>();
  readConfigFile(vm["configfile"].as<std::string>(), vm);

  WorkerSettings settings = parseWorkerSettings(vm);

  init::initialiseSignalHandler();

  LOG(INFO)<< "Reading job types";
  std::vector<uint> jobList = jobTypesToValues(io::split(vm["jobtypes"].as<std::string>(), ','));

  LOG(INFO)<< "Getting problem specification from delegator";
  stateline::comms::Worker worker(jobList, settings);

  LOG(INFO)<< "Decoding world specification";
  WorldSpec worldSpec;
  obsidian::comms::unserialise(worker.globalSpec(), worldSpec);
  std::vector<world::InterpolatorSpec> boundaryInterp = world::worldspec2Interp(worldSpec);

  std::set<ForwardModel> enabled;
  for (uint i : worker.jobsEnabled())
    enabled.insert(static_cast<ForwardModel>(i));

  if (enabled.empty())
  {
    LOG(ERROR) << "No job types to run among specified: " << vm["jobtypes"].as<std::string>();
    exit(EXIT_FAILURE);
  }
  std::vector<std::future<bool>> threads;
  applyToSensorsEnabled<launchWorkerThread>(enabled, std::ref(threads), std::ref(worker), std::ref(worldSpec), std::ref(boundaryInterp),
                                            std::cref(vm));
  // Wait for the other threads to terminate
  for (auto& t : threads)
  {
    t.wait();
  }
  LOG(INFO)<< "All threads done";

  // Stop the worker cleanly
  worker.stop();

  return 0;
}
