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

#include <string>
#include <vector>
#include <fstream>
#include <cassert>

#include "settings.hpp"
#include "../infer/datatypes.hpp"

#include <iostream>

namespace stateline
{
  namespace db
  {
    std::ostream& operator<<(std::ostream& os, const mcmc::State& s);

    // Database writer interface
    class CSVChainArrayWriter
    {
      public:
        CSVChainArrayWriter(const std::string& directory, uint numChains);
        void append(int id, std::vector<mcmc::State>::iterator start,
          std::vector<mcmc::State>::iterator end);

      private:
        std::vector<std::fstream> chainFiles_;
    };

  } // namespace db
} // namespace stateline
