#pragma once

#include "ezOptionParser/ezOptionParser.hpp"
#include <iostream>

namespace stateline
{
  bool parseCommandLine(ez::ezOptionParser& opt, int argc, const char* argv[])
  {
    opt.parse(argc, argv);

    if (opt.isSet("-h"))
    {
      std::string usage;
      opt.getUsage(usage);
      std::cout << usage;
      return false;
    }

    return true;
  }

}
