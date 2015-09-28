//! Simple logger class used by the delegator
//!
//! \file logging.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \licence Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "infer/logging.hpp"
#include <easylogging/easylogging++.h>

namespace stateline
{
  namespace mcmc
  {
    TableLogger::TableLogger(uint nstacks, uint nchains, uint ndims, uint msRefresh)
          : msRefresh_(msRefresh),
            nstacks_(nstacks), 
            nchains_(nchains), 
            lengths_(nstacks*nchains),
            minEnergies_(nstacks*nchains),
            energies_(nstacks*nchains),
            nAcceptsGlobal_(nstacks*nchains),
            nSwapsGlobal_(nstacks*nchains),
            nSwapAttemptsGlobal_(nstacks*nchains),
            diagnostic_(nstacks, nchains, ndims)
    {
      std::fill(lengths_.begin(), lengths_.end(), 1);
      std::fill(minEnergies_.begin(), minEnergies_.end(), std::numeric_limits<double>::infinity());
      std::fill(nAcceptsGlobal_.begin(), nAcceptsGlobal_.end(), 1);
      std::fill(nSwapAttemptsGlobal_.begin(), nSwapAttemptsGlobal_.end(), 1); // for nan-free behaviour
    }

    void TableLogger::update(uint id, const State& s,
        const std::vector<double>& sigmas,
        const std::vector<double>& acceptRates,
        const std::vector<double>& betas,
        const std::vector<double>& swapRates)
    {
      // update the variables
      lengths_[id] += 1;
      minEnergies_[id] = std::min(minEnergies_[id], s.energy);
      energies_[id] = s.energy;
      nAcceptsGlobal_[id] += s.accepted;
      nSwapsGlobal_[id] += s.swapType == SwapType::Accept;
      nSwapAttemptsGlobal_[id] += s.swapType != SwapType::NoAttempt;
      diagnostic_.update(id, s);

      if (ch::duration_cast<ch::milliseconds>(ch::steady_clock::now() - lastPrintTime_).count() > msRefresh_)
      {
        // Print to the screen
        lastPrintTime_ = ch::steady_clock::now();

        // Create a string stream that formats the data nicely in a table
        std::stringstream ss;
        ss.fill(' ');
        ss.precision(5);
        ss.setf(std::ios::showpoint | std::ios::fixed);

        ss << "\n\n  ID    Length    MinEngy   CurrEngy      Sigma     AcptRt  GlbAcptRt       Beta     SwapRt  GlbSwapRt\n";
        ss << "------------------------------------------------------------------------------------------------------\n";
        for (uint i = 0; i < lengths_.size(); i++)
        {
          if (i % nchains_ == 0 && i != 0)
            ss << '\n';

          ss << std::setw(4) << i << " "
             << std::setw(9) << lengths_[i] << " "
             << std::setw(10) << minEnergies_[i] << " "
             << std::setw(10) << energies_[i] << " "
             << std::setw(10) << sigmas[i] << " "
             << std::setw(10) << acceptRates[i] << " "
             << std::setw(10) << nAcceptsGlobal_[i] / (double) lengths_[i] << " "
             << std::setw(10) << betas[i] << " "
             << std::setw(10) << swapRates[i] << " " << std::setw(10)
             << nSwapsGlobal_[i] / (double) nSwapAttemptsGlobal_[i] << " \n";
        }

        std::cout << ss.str() << std::endl;

        std::cout << "Convergence test: " << diagnostic_.rHat().mean() << " (" <<
                     (diagnostic_.hasConverged() ? "possibly converged" :
                                                   "not converged")
                     << ")" << std::endl;;
      }
    }
  } // namespace mcmc
} // namespace stateline
