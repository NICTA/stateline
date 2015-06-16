#pragma once

#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;
// Parse the command line 

namespace stateline
{

  po::variables_map parseCommandLine(int ac, char* av[], 
      const po::options_description& opts)
  {
    po::variables_map vm;
    try
    {
      po::store(po::parse_command_line(ac, av, opts), vm);
      po::notify(vm);
    } catch (const std::exception& ex)
    {
      std::cout << "Error: Unrecognised commandline argume" << opts << "\n";
      exit(EXIT_FAILURE);
    }
    return vm;
  }

}
