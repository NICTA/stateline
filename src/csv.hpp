//!
//! Reading and writing from CSV.
//!
//! \file csv.hpp
//! \author Darren Shen
//! \date 2015
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2015, NICTA
//!

#include <string>

#include "infer/samplesarray.hpp"

namespace stateline
{
  void saveToCSV(const mcmc::SamplesArray& samples, const std::string& directory);
}
