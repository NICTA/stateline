//!
//! Dynamic array of MCMC samples.
//!
//! \file infer/samplesarray.hpp
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "../infer/datatypes.hpp"

namespace stateline
{
  namespace mcmc
  {
    class SamplesArray
    {
      public:
        SamplesArray(uint numColdChains);

        std::vector<State> chain(uint chainIndex) const;

        const State &at(uint chainIndex, uint sampleIndex) const;

        std::vector<State> operator[](uint chainIndex) const;

        void append(uint chainIndex, const State& state);

      private:
        std::vector<std::vector<State>> states_;
    };
  }
}
