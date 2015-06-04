//!
//! Read-only array of MCMC samples.
//!
//! \file infer/sampler.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "../infer/samplesarray.hpp"

namespace stateline
{
  namespace mcmc
  {
    std::vector<State> SamplesArray::chain(uint chainIndex) const
    {
      assert(chainIndex < states_.size());
      return states_[chainIndex];
    }

    const State& SamplesArray::at(uint chainIndex, uint sampleIndex) const
    {
      assert(chainIndex < states_.size());
      assert(states_[chainIndex].size() < sampleIndex);
      return states_[chainIndex][sampleIndex];
    }

    std::vector<State> SamplesArray::operator[](uint chainIndex) const
    {
      assert(chainIndex < states_.size());
      return states_[chainIndex];
    }

    SamplesArray::SamplesArray(uint numColdChains)
      : states_(numColdChains)
    {
    }

    void SamplesArray::setChain(uint chainIndex, const std::vector<State>& states)
    {
      assert(chainIndex < states_.size());

      states_[chainIndex].push_back(state);
    }
  }
}
