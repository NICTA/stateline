//!
//! Contains implementation of the database structure used for persistent
//! storage of the MCMC data.
//!
//! \file db/db.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "db/db.hpp"

namespace stateline
{
  namespace db
  {
    std::ostream& operator<<(std::ostream& os, const mcmc::State& s)
    {
      for (uint i = 0; i < s.sample.size(); i++) {
        os << s.sample(i) << ",";
      }
      os << s.energy << "," << s.sigma << "," << s.beta << "," << s.accepted << "," << (int)s.swapType;
      return os;
    }

    CSVChainArrayWriter::CSVChainArrayWriter(const std::string& directory, uint numChains)
          : chainFiles_(numChains)
    {
      for (uint i = 0; i < numChains; i++) {
        chainFiles_[i].open(directory + "/" + std::to_string(i) + ".csv",
            std::fstream::in | std::fstream::out | std::fstream::trunc);
        assert(chainFiles_[i].good());
      }
    }

    void CSVChainArrayWriter::append(int id, std::vector<mcmc::State>::iterator start,
        std::vector<mcmc::State>::iterator end)
    {
      // TODO: needs to be transactional
      assert(id >= 0 && id < chainFiles_.size());
      
      std::for_each(start, end, [&](const mcmc::State &s){
          chainFiles_[id] << s << "\n";});
    
      chainFiles_[id] << std::flush;
    }
  } // namespace db
} // namespace stateline

