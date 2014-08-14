//!
//! Contains the interface for logging MCMC.
//!
//! \file infer/logging.hpp
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <limits>
#include <chrono>

#include "infer/state.hpp"

namespace stateline
{
  namespace mcmc
  {
    /*
    //! A logger which does nothing.
    //!
    class NoLogger
    {
      public:
        void update(uint id, const State &state)
        {
        }
    };

    //! Temporary simple logger to stdout
    //!
    class SimpleLogger
    {
      public:
        SimpleLogger(const MCMCSettings &s, std::chrono::seconds::rep frequency)
          : lengths_(s.stacks * s.chains, 0),
            sigmas_(s.stacks * s.chains, 0.0),
            betas_(s.stacks * s.chains, 0.0),
            acceptRates_(s.stacks * s.chains, 0.0),
            swapRates_(s.stacks * s.chains, 0.0),
            lowestEnergies_(s.stacks * s.chains, std::numeric_limits<double>::infinity()),
            frequency_(std::chrono::seconds(frequency)),
            nextOutputTime_(std::chrono::steady_clock::now() + frequency_)
        {
        }

        void update(uint id, const State &state)
        {
          sigmas_[id] = state.sigma;
          betas_[id] = state.beta;
          acceptRates_[id] = (lengths_[id] * acceptRates_[id] + state.accepted) /
                            (lengths_[id] + 1);
          swapRates_[id] = (lengths_[id] * swapRates_[id] + (state.swapType == SwapType::Accept)) /
                          (lengths_[id] + 1);
          lowestEnergies_[id] = std::min(lowestEnergies_[id], state.energy);
          lengths_[id]++;

          if (std::chrono::steady_clock::now() > nextOutputTime_)
          {
            std::cout << "\n\nChainID Length MinEngy CurEngy Sigma GlbAcptRt Beta GlbSwapRt" << std::endl;
            std::cout << "----------------------------------------------------------------------------------------------" << std::endl;
            for (std::size_t i = 0; i < lengths_.size(); i++)
            {
              std::cout << i << " " <<
                std::setw(9) << lengths_[i] << " " <<
                std::setw(10) << lowestEnergies_[i] << " " <<
                std::setw(10) << sigmas_[i] << " " <<
                std::setw(10) << acceptRates_[i] << " " <<
                std::setw(10) << betas_[i] << " " <<
                std::setw(10) << swapRates_[i] << std::endl;
            }

            nextOutputTime_ = std::chrono::steady_clock::now() + frequency_;
          }
        }

      private:
        std::vector<std::size_t> lengths_;
        std::vector<double> sigmas_;
        std::vector<double> betas_;
        std::vector<double> acceptRates_;
        std::vector<double> swapRates_;
        std::vector<double> lowestEnergies_;
        std::chrono::seconds frequency_;
        std::chrono::time_point<std::chrono::steady_clock> nextOutputTime_;
    };*/
  }
}
