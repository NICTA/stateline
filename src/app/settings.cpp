//!
//! \file app/settings.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "settings.hpp"

#include <fstream>
#include <boost/filesystem.hpp>
#include <glog/logging.h>

namespace obsidian
{
  po::options_description configFileOptions()
  {
    po::options_description configFile("GDF Configuration File Options");
    configFile.add_options() //
    ("worker.pollRate", po::value<int>()->default_value(100), "time in ms between network polls") //
    ("worker.heartbeatRate", po::value<uint>()->default_value(1000), "time in ms between heartbeats") //
    ("worker.heartbeatPollRate", po::value<uint>()->default_value(500), "time in ms between heartbeat polls") //
    ("worker.heartbeatTimeout", po::value<uint>()->default_value(3000), "time in ms for no hearbeat disconnection") //
    ("delegator.pollRate", po::value<int>()->default_value(100), "time in ms between network polls") //
    ("delegator.heartbeatRate", po::value<uint>()->default_value(1000), "time in ms between heartbeats") //
    ("delegator.heartbeatPollRate", po::value<int>()->default_value(500), "time in ms between heartbeat polls") //
    ("delegator.heartbeatTimeout", po::value<uint>()->default_value(10000), "time in ms for no hearbeat disconnection") //
    ("database.directory", po::value<std::string>()->default_value("chainDB"), "Directory for storing chain database") //
    ("database.cacheSizeMB", po::value<double>()->default_value(100.0), "Size of database cache in MB");
    return configFile;
  }

  void readConfigFile(const std::string& configFilename, po::variables_map& vm)
  {
    po::options_description configFile("Configuration File Options");
    configFile.add(configFileOptions());
    LOG(INFO)<< "Reading config from " << configFilename;
    boost::filesystem::path file(configFilename);
    if (!boost::filesystem::exists(file))
    {
      LOG(INFO)<< "Config file " << configFilename << " not found, proceeding with defaults";
    }
    std::ifstream ifsCfg(configFilename);
    po::store(po::parse_config_file(ifsCfg, configFile), vm);
    po::notify(vm);
  }

  stateline::DBSettings parseDBSettings(const po::variables_map& vm)
  {
    LOG(INFO)<< "Parsing database settings";
    stateline::DBSettings s;
    s.directory = vm["database.directory"].as<std::string>();
    s.recover = vm["recover"].as<bool>();
    s.cacheSizeMB = vm["database.cacheSizeMB"].as<double>();
    return s;
  }

  stateline::DelegatorSettings parseDelegatorSettings(const po::variables_map& vm)
  {
    LOG(INFO)<< "Parsing delegator settings";
    stateline::HeartbeatSettings delHb;
    delHb.msRate = vm["delegator.heartbeatRate"].as<uint>();
    delHb.msPollRate = vm["delegator.heartbeatPollRate"].as<int>();
    delHb.msTimeout = vm["delegator.heartbeatTimeout"].as<uint>();
    stateline::DelegatorSettings del;
    del.heartbeat = delHb;
    del.msPollRate = vm["delegator.pollRate"].as<int>();
    del.port = vm["port"].as<uint>();
    return del;
  }

  stateline::WorkerSettings parseWorkerSettings(const po::variables_map& vm)
  {
    LOG(INFO)<<"Parsing worker settings";
    stateline::HeartbeatSettings wkHb;
    wkHb.msRate = vm["worker.heartbeatRate"].as<uint>();
    wkHb.msPollRate = vm["worker.heartbeatPollRate"].as<uint>();
    wkHb.msTimeout = vm["worker.heartbeatTimeout"].as<uint>();
    stateline::WorkerSettings wk;
    wk.heartbeat = wkHb;
    wk.msPollRate = vm["worker.pollRate"].as<int>();
    wk.address = vm["address"].as<std::string>();
    return wk;
  }
}
