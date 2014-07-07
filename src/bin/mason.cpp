//!
//! Output the voxelisations of an obsidian run in open format.
//!
//! \file mason.cpp
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
#include "fwdmodel/global.hpp"
#include "datatype/sensors.hpp"
#include "io/npy.hpp"
#include "io/dumpnpz.hpp"
#include "likelihood/likelihood.hpp"
#include "world/property.hpp"
#include "detail.hpp"

using namespace obsidian;
using namespace stateline;

namespace ph = std::placeholders;

// Command line options specific to obsidian
po::options_description commandLineOptions()
{
  po::options_description cmdLine("Delegator Command Line Options");
  cmdLine.add_options()
    ("configfile,c", po::value<std::string>()->default_value("obsidian_config"), "configuration file")
    ("xres,x", po::value<uint>()->default_value(32), "Resolution of the voxelisation in the x direction")
    ("yres,y", po::value<uint>()->default_value(24), "Resolution of the voxelisation in the y direction")
    ("zres,z", po::value<uint>()->default_value(30), "Resolution of the voxelisation in the z direction")
    ("blocksize,b", po::value<uint>()->default_value(100), "Number of samples in each block")
    ("inputfile,i", po::value<std::string>()->default_value("input.obsidian"), "input file")
    ("pickaxefile,p", po::value<std::string>()->default_value("output.npz"), "output file from pickaxe")
    ("outputfile,o", po::value<std::string>()->default_value("voxels.npz"), "output file")
    ("recover,r", po::bool_switch()->default_value(true), "recovery (deprecated)");
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

  readConfigFile(vm["configfile"].as<std::string>(), vm);
  readInputFile(vm["inputfile"].as<std::string>(), vm);
  std::set<ForwardModel> sensorsEnabled = parseSensorsEnabled(vm);
  DBSettings dbSettings = parseDBSettings(vm);
  dbSettings.recover = true;
  GlobalPrior prior = parsePrior<GlobalPrior>(vm, sensorsEnabled);
  
  // Stuff needed for forward modelling
  GlobalSpec globalSpec = parseSpec<GlobalSpec>(vm, sensorsEnabled);
  std::vector<world::InterpolatorSpec> boundaryInterp = world::worldspec2Interp(globalSpec.world);
  GlobalResults realResults = loadResults(globalSpec.world, vm, sensorsEnabled);
  GlobalCache cache = fwd::generateGlobalCache(boundaryInterp, globalSpec, sensorsEnabled);

  std::string filename = vm["outputfile"].as<std::string>();
  std::string extension = ".npz";
  auto extfound = filename.rfind(extension);
  if (extfound!=std::string::npos)
    filename.erase(extfound,extension.length());

  // Get the resolution
  uint xres = vm["xres"].as<uint>(), yres = vm["yres"].as<uint>(), zres = vm["zres"].as<uint>();

  // Create the query over this grid
  world::Query query(boundaryInterp, globalSpec.world, xres, yres, zres, world::SamplingStrategy::noAA);

  // Read the output thetas from pickaxe
  Eigen::MatrixXd thetas = io::readNPZ(vm["pickaxefile"].as<std::string>(), "thetas");
  uint blocksize = vm["blocksize"].as<uint>();
  uint nsamples = thetas.rows();
  uint nfiles = (nsamples / blocksize) + (int)(nsamples%blocksize!=0);
  uint nlayers = globalSpec.world.boundaries.size();
  uint trueSize = std::min(blocksize, nsamples);

  LOG(INFO) << "block size:" << blocksize;
  LOG(INFO) << "number of samples:" << nsamples;
  LOG(INFO) << "number of files:" << nfiles;

  std::vector<Eigen::MatrixXd> dumpProps((uint)RockProperty::Count);
  std::vector<Eigen::MatrixXd> dumpLayers(nlayers);
  std::vector<Eigen::MatrixXd> dumpTransitions(nlayers);

  // Initialise the matrices
  for (uint i = 0; i < dumpProps.size(); i++)
  {
    dumpProps[i] = Eigen::MatrixXd(trueSize, xres * yres * zres);
  }

  for (uint i = 0; i < dumpLayers.size(); i++)
  {
    dumpLayers[i] = Eigen::MatrixXd(trueSize, xres * yres * zres);
  }
  for (uint i = 0; i < dumpLayers.size(); i++)
  {
    dumpTransitions[i] = Eigen::MatrixXd(trueSize, xres * yres);
  }

  // Write to each file
  uint sample = 0;
  for (uint file = 0; file < nfiles; file++)
  {
    LOG(INFO) << " Processing block " << file;
    io::NpzWriter writer(filename + std::to_string(file) + ".npz");
    writer.write<double>("resolution", Eigen::Vector3d(query.resX, query.resY, query.resZ));
    writer.write<double>("x_bounds", Eigen::Vector2d(globalSpec.world.xBounds.first, globalSpec.world.xBounds.second));
    writer.write<double>("y_bounds", Eigen::Vector2d(globalSpec.world.yBounds.first, globalSpec.world.yBounds.second));
    writer.write<double>("z_bounds", Eigen::Vector2d(globalSpec.world.zBounds.first, globalSpec.world.zBounds.second)); 

    for (uint i = 0; i < blocksize && sample < nsamples; i++, sample++)
    {
      if (global::interruptedBySignal)
      {
        break;
      }


      for (uint j = 0; j < (uint)RockProperty::Count; j++)
      {
        // Reconstruct world model
        Eigen::VectorXd theta = thetas.row(sample);
        GlobalParams params = prior.reconstruct(theta);

        // Extract only this rock property
        Eigen::VectorXd props = world::extractProperty(params.world, (RockProperty)j);

        Eigen::MatrixXd transitions = getTransitions(boundaryInterp, params.world, query);
        Eigen::MatrixXd voxels = world::voxelise(transitions, query.edgeZ, props);

        dumpProps[j].row(i) = world::flatten(voxels).transpose();
      }


      for (uint j = 0; j < nlayers; j++)
      {
        // Reconstruct world model
        Eigen::VectorXd theta = thetas.row(sample);
        GlobalParams params = prior.reconstruct(theta);

        // Show only this particular layer in the voxelisation
        Eigen::VectorXd props = Eigen::VectorXd::Zero(nlayers);
        props(j) = 1.0;

        Eigen::MatrixXd transitions = getTransitions(boundaryInterp, params.world, query);
        Eigen::MatrixXd voxels = world::voxelise(transitions, query.edgeZ, props);

        dumpLayers[j].row(i) = world::flatten(voxels).transpose();
        dumpTransitions[j].row(i) = transitions.row(j);
      }
    }

    LOG(INFO) << "Writing file..." << file;
    // Dump them
    dumpPropVoxelsNPZ(writer, dumpProps);
    dumpTransitionsVoxelsNPZ(writer, dumpTransitions);
    dumpLayerVoxelsNPZ(writer, dumpLayers);
  }

  return 0;
}
