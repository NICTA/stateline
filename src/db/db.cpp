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

#include <glog/logging.h>

#include "leveldb/write_batch.h"
#include "leveldb/cache.h"
#include "leveldb/options.h"
#include "leveldb/filter_policy.h"

namespace stateline
{
  namespace db
  {
    std::ostream& operator<<(std::ostream& os, const mcmc::State& s)
    {
      for (uint i = 0; i < s.sample.size(); i++) {
        os << s.sample(i) << ",";
      }
      os << s.sample << "," << s.energy << "," << s.sigma << "," << s.beta << "," << s.accepted << "," << (int)s.swapType;
    }

    CSVChainArrayWriter::CSVChainArrayWriter(const std::string& directory, uint numChains)
          : chainFiles_(numChains), lastLinePos_(numChains)
    {
      for (uint i = 0; i < numChains; i++) {
        chainFiles_[i].open(directory + "/" + std::to_string(i) + ".csv",
            std::fstream::in | std::fstream::out | std::fstream::trunc);
        assert(chainFiles_[i].is_open());
      }
    }

    void CSVChainArrayWriter::append(int id, const std::vector<mcmc::State>& states)
    {
      // TODO: needs to be transactional
      assert(id >= 0 && id < chainFiles_.size());

      for (const auto& state : states)
      {
        lastLinePos_[id] = chainFiles_[id].tellg();
        chainFiles_[id] << state << "\n";
      }

      chainFiles_[id] << std::flush; // Flush to disk
    }

    void CSVChainArrayWriter::replaceLast(int id, const mcmc::State& state)
    {
      // PRECONDITION: just flushed
      chainFiles_[id].seekg(lastLinePos_[id]);
      chainFiles_[id] << state << "\n";
    }

  } // namespace db
} // namespace stateline

