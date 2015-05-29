//!
//! A demo using Stateline to sample from a Gaussian mixture.
//!
//! \file stateline.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \licence Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <boost/program_options.hpp>
#include <json.hpp>


#include "../app/serverwrapper.hpp"
#include "../app/logging.hpp"
#include "../app/serial.hpp"
#include "../app/signal.hpp"
#include "../app/commandline.hpp"

// Alias namespaces for conciseness
namespace sl = stateline;
namespace ph = std::placeholders;
namespace po = boost::program_options;
namespace ch = std::chrono;
using json = nlohmann::json;

po::options_description commandLineOptions()
{
  auto opts = po::options_description("Demo Options");
  opts.add_options()
  ("loglevel,l", po::value<int>()->default_value(0), "Logging level")
  ("port,p",po::value<uint>()->default_value(5555), "Port on which to accept worker connections") 
  ("config,c",po::value<std::string>()->default_value("config.json"), "Path to configuration file")
  ;
  return opts;
}

json initConfig(const po::variables_map& vm)
{
  std::ifstream configFile(vm["config"].as<std::string>());
  if (!configFile)
  {
    // TODO: use default settings?
    LOG(FATAL) << "Could not find config file";
  }

  json config;
  configFile >> config;
  return config;
}

int main(int ac, char *av[])
{
  po::variables_map vm = sl::parseCommandLine(ac, av, commandLineOptions());
  json config = initConfig(vm);
  uint port = vm["port"].as<uint>();
  sl::StatelineSettings settings = sl::StatelineSettings::fromJSON(config);

  sl::init::initialiseSignalHandler();
  sl::initLogging("server", vm["loglevel"].as<int>(), true, "");

  sl::ServerWrapper s(port, settings);
  
  s.start();  

  while(!sl::global::interruptedBySignal)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  s.stop();

  // Load the chains here from CSV?

  return 0;


}
