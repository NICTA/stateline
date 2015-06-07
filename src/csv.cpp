//!
//! Reading and writing from CSV.
//!
//! \file csv.hpp
//! \author Darren Shen
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2015, NICTA
//!

#include "csv.hpp"

#include <fstream>

namespace stateline
{
  std::ostream& operator<<(std::ostream& os, const mcmc::State& s)
  {
    for (uint i = 0; i < s.sample.size(); i++)
    {
      os << s.sample(i) << ",";
    }
    os << s.energy << "," << s.sigma << "," << s.beta << "," << s.accepted << "," << (int)s.swapType;
    return os;
  }

  void saveToCSV(const mcmc::SamplesArray& samples, const std::string& directory)
  {
    std::vector<std::ofstream> files;
    for (uint i = 0; i < samples.numChains(); i++) {
      files.emplace_back(directory + "/" + std::to_string(i) + ".csv");
      assert(files.back().good());
    }

    for (uint i = 0; i < samples.numChains(); i++)
    {
      for (const auto& state : samples.chain(i))
      {
        files[i] << state << "\n";
      }
    }
  }
}
