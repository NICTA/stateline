//!
//! Layer for setup of console applications; signal handlers and logging.
//!
//! \file app/console.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

// Standard Library
#include <string>
// Prerequisites
#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace obsidian
{
  namespace init
  {
    /**
     * Initialise the (GLog) logging system
     *
     * \param appName The name of the executable
     * \param logLevel The numerical level of logging. <0 gets increasingly
     *        verbose for debugging. 0 is release logging, and >0 is 
     *        increasingly quiet.
     * \param stdErr Switch to enable logging to stdError
     * \param directory The directory to log to if stdErr logging is disabled
     *
     */
    void initialiseLogging(std::string appName, int logLevel, bool stdErr, std::string directory);

    /**
     * Initialise (commandline/config file) program options
     *
     * \param argc The number of args given on the commandline
     * \param argv The args from the commandline
     * \param options A vector of program-specific options
     * \return The variable map containing the values of the given options
     */
    po::variables_map initProgramOptions(int argc, char* argv[], const po::options_description& cmdLineOptions);

  } // namespace init
} // namespace obsidian
