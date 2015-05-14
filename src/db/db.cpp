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
#include <iomanip>
#include <cstdlib>

#include "leveldb/write_batch.h"
#include "leveldb/cache.h"
#include "leveldb/options.h"
#include "leveldb/filter_policy.h"

namespace stateline
{
  namespace db
  {
    /*
    std::ostream& operator<<(std::ostream& os, const mcmc::State& s)
    {
      for (uint i = 0; i < s.sample.size(); i++) {
        os << s.sample(i) << ",";
      }
      //os << "1";
      //os << s.energy << "," << s.sigma << "," << s.beta << "," << s.accepted << "," << (int)s.swapType;
      return os;
    }*/

    CSVChainArrayWriter::CSVChainArrayWriter(const std::string& directory, uint numChains)
          : chainFiles_(numChains), lastLinePos_(numChains)
    {
      for (uint i = 0; i < numChains; i++) {
        chainFiles_[i].open(directory + "/" + std::to_string(i) + ".csv");//,
            ///*std::fstream::in |*/ std::fstream::out | std::fstream::trunc);
        chainFiles_[i].exceptions(std::ios::failbit);
        assert(chainFiles_[i].good());
      }
    }

    CSVChainArrayWriter::~CSVChainArrayWriter()
    {
      for (int i = 0; i < chainFiles_.size(); i++)
        chainFiles_[i].close();
    }

    void CSVChainArrayWriter::append(int id, const std::vector<mcmc::State>& states)
    {
      // TODO: needs to be transactional
      assert(id >= 0 && id < chainFiles_.size());

      static int rand = 0;

      //for (const auto& state : states)
      for (int j = 0; j < states.size(); j++)
      {
        const mcmc::State& state = states[j];
        //lastLinePos_[id] = chainFiles_[id].tellg();
        //for (uint i = 0; i < state.sample.size(); i++) {
        for (uint i = 0; i < 3; i++) {
          rand = (rand * 3 + 17) % 997;
          int a = rand;//state.sample(i);
          //int a = 999;
          chainFiles_[id] << std::to_string(a) << ",";
          //chainFiles_[id] << a << ",";
        }
        chainFiles_[id] << "NEWLINE" << std::endl;
      }

      chainFiles_[id] << "ENDFLUSH" << std::endl; // Flush to disk
      //chainFiles_[id] << "ENDFLUSH\n";
    }

    void CSVChainArrayWriter::replaceLast(int id, const mcmc::State& state)
    {
      // PRECONDITION: just flushed
      //chainFiles_[id].seekg(lastLinePos_[id]);
      //chainFiles_[id] << state << "\n";
    }

  } // namespace db
} // namespace stateline

