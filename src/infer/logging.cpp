//! Simple logger class used by the delegator
//!
//! \file logging.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \licence Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "infer/logging.hpp"

namespace stateline
{
  namespace mcmc
  {
    Logger::Logger(uint nstacks, uint nchains, uint msRefresh)
          : msRefresh_(msRefresh),
            nstacks_(nstacks), 
            nchains_(nchains), 
            lengths_(nstacks*nchains),
            minEnergies_(nstacks*nchains),
            energies_(nstacks*nchains),
            nAcceptsGlobal_(nstacks*nchains),
            nSwapsGlobal_(nstacks*nchains),
            nSwapAttemptsGlobal_(nstacks*nchains)
    {
      std::fill(lengths_.begin(), lengths_.end(), 1);
      std::fill(minEnergies_.begin(), minEnergies_.end(), std::numeric_limits<double>::infinity());
      std::fill(nAcceptsGlobal_.begin(), nAcceptsGlobal_.end(), 1);
      std::fill(nSwapAttemptsGlobal_.begin(), nSwapAttemptsGlobal_.end(), 1); // for nan-free behaviour
    }

    void Logger::update(uint id, const State& s,
        const std::vector<Eigen::VectorXd>& sigmas,
        const std::vector<Eigen::VectorXd>& acceptRates,
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
             << std::setw(10) << sigmas[i](0) << " "
             << std::setw(10) << acceptRates[i](0) << " "
             << std::setw(10) << nAcceptsGlobal_[i] / (double) lengths_[i] << " "
             << std::setw(10) << betas[i] << " "
             << std::setw(10) << swapRates[i] << " " << std::setw(10)
             << nSwapsGlobal_[i] / (double) nSwapAttemptsGlobal_[i] << " \n";
        }

        std::cout << ss.str() << std::endl;
      }
    }
  } // namespace mcmc
} // namespace stateline
