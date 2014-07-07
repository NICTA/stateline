//!
//! \file app/console.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

// Standard Library
#include <vector>
#include <cstdlib>
#include <iostream>
// Prerequisites
#include <boost/program_options.hpp>
#include <boost/program_options/errors.hpp>
#include <glog/logging.h>
// Project
#include "io/string.hpp"
#include "app/console.hpp"

namespace obsidian
{
  namespace init
  {

    void initialiseLogging(std::string appName, int logLevel, bool stdErr, std::string directory)
    {
      int normLog = logLevel;
      int vLevel = 0;
      if (logLevel < 0)
      {
        vLevel = -logLevel;
        normLog = 0;
      }

      google::InitGoogleLogging(appName.c_str());
      FLAGS_logtostderr = stdErr;
      FLAGS_log_dir = directory;
      FLAGS_minloglevel = normLog;
      FLAGS_v = vLevel;
      LOG(INFO)<< "Logging initialised with level " << normLog << " and verbosity " << vLevel;
    }

    po::options_description genericCommandLineOptions()
    {
      po::options_description genericCmdLine("Generic Command Line Options");
      genericCmdLine.add_options() //
      ("version,v", "print version string") //
      ("help,h", "produce help message") //
      ("loglevel,l", po::value<int>()->default_value(2), "set logging level\n(<0=DEBUG, 0=INFO, 1=WARNING, 2=ERROR, 3=FATAL)") //
      ("logdir,d", po::value<std::string>()->default_value("."), "logging directory") //
      ("silent,s", "don't log to stderr");
      return genericCmdLine;
    }

    po::variables_map initProgramOptions(int argc, char* argv[], const po::options_description& cmdLineOptions)
    {
      // Create the global options object
      po::options_description cmdLine("Command-Line Options");

      // Create the generic config options for all GDF apps
      cmdLine.add(genericCommandLineOptions());
      // Add the other options specifically defined
      cmdLine.add(cmdLineOptions);

      // Parse the command line 
      po::variables_map vm;
      try
      {
        po::store(po::parse_command_line(argc, argv, cmdLine), vm);
        po::notify(vm);
      } catch (const std::exception& ex)
      {
        std::cout << "Error: Unrecognised commandline arguments\n\n" << cmdLine << "\n";
        exit(EXIT_FAILURE);
      }

      // Act on the generic options
      if (vm.count("version"))
      {
        std::cout << "Version 0.1\n";
        exit(EXIT_SUCCESS);
      }
      if (vm.count("help"))
      {
        std::cout << cmdLine << "\n";
        exit(EXIT_SUCCESS);
      }
      // Initialise logging
      bool silent = vm.count("silent");
      initialiseLogging(argv[0], vm["loglevel"].as<int>(), !silent, vm["logdir"].as<std::string>());
      return vm;
    }

  } // namespace init
} // namespace obsidian
