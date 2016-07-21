//! Runs the MCMC and delegator.
//!
//! \file src/bin/server.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \licence Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <json.hpp>

#include "ezoptionparser/ezOptionParser.hpp"

#include "app/serverwrapper.hpp"
#include "app/logging.hpp"
#include "app/signal.hpp"
#include "app/commandline.hpp"

namespace sl = stateline;
using json = nlohmann::json;

ez::ezOptionParser commandLineOptions()
{
  ez::ezOptionParser opt;
  opt.overview = "Demo options";
  opt.add("", 0, 0, 0, "Print help message", "-h", "--help");
  opt.add("0", 0, 1, 0, "Logging level", "-l", "--log-level");
  opt.add("5555", 0, 1, 0, "Port on which to accept worker connections", "-p", "--port");
  opt.add("config.json", 0, 1, 0, "Path to configuration file", "-c", "--config");
  return opt;
}

json initConfig(const std::string& configPath)
{
  std::ifstream configFile(configPath);
  if (!configFile)
  {
    // TODO: use default settings?
    LOG(FATAL) << "Could not find config file";
  }

  json config;
  configFile >> config;
  return config;
}

int main(int argc, const char *argv[])
{
  // Parse the command line
  auto opt = commandLineOptions();
  if (!sl::parseCommandLine(opt, argc, argv))
    return 0;

  // Initialise logging
  int logLevel;
  opt.get("-l")->getInt(logLevel);
  sl::initLogging(logLevel);

  std::string configPath;
  opt.get("-c")->getString(configPath);
  const auto config = initConfig(configPath);
  sl::StatelineSettings settings = sl::StatelineSettings::fromJSON(config);

  int port;
  opt.get("-p")->getInt(port);

  sl::ServerWrapper s{port, settings};
  s.start();

  while(s.isRunning())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  s.stop();

  return 0;
}
