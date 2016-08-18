//!
//! Contains the interface used for persistent storage of the MCMC data.
//!
//! \file db/db.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "infer/datatypes.hpp"

#include <string>
#include <vector>

#include "H5Cpp.h"

namespace stateline { namespace db {

struct DBSettings
{
  std::string filename;
  std::size_t numChains;
  std::size_t numDims;
  std::size_t chunkSize;
};

// Database writer interface
class DBWriter
{
public:
  DBWriter(const DBSettings& settings);
  void appendStates(std::size_t id, std::vector<mcmc::State>::iterator start,
    std::vector<mcmc::State>::iterator end);

private:
  DBSettings settings_;
  H5::H5File file_;
  std::vector<H5::Group> groups_;
  std::vector<H5::DataSet> datasets_;
  std::vector<double> sbuffer_;
};

} }
