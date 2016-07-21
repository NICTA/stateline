//! Process that runs the Stateline agent
//!
//! \file src/bin/agent.cpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \licence Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <iostream>
#include <functional>
#include <string>

#include "ezoptionparser/ezOptionParser.hpp"

#include "app/logging.hpp"
#include "app/commandline.hpp"
#include "comms/agent.hpp"

namespace sl = stateline;

ez::ezOptionParser commandLineOptions()
{
  ez::ezOptionParser opt;
  opt.overview = "Stateline agent options";
  opt.add("", 0, 0, 0, "Print help message", "-h", "--help");
  opt.add("0", 0, 1, 0, "Logging level", "-l", "--log-level");
  opt.add("localhost:5555", 0, 1, 0, "Address of delegator", "-n", "--network-addr");
  opt.add("ipc:///tmp/sl_agent.sock", 0, 1, 0, "Address of agent for worker to connect to", "-a", "--agent-addr");
  return opt;
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

  // Initialise the agent
  std::string networkAddr, bindAddr;
  opt.get("-a")->getString(bindAddr);
  opt.get("-n")->getString(networkAddr);
  sl::comms::AgentSettings settings{bindAddr, "tcp://" + networkAddr};

  zmq::context_t ctx{1};
  sl::comms::Agent agent{ctx, settings};

  bool running = true;
  agent.start(running);

  return 0;
}
