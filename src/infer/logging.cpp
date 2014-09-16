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

    Logger::Logger(uint nstacks, uint nchains, uint msRefresh, 
            const std::vector<Eigen::VectorXd>& sigmas, const std::vector<double>& acceptRates,
            const std::vector<double>& betas, const std::vector<double>& swapRates)
          : msRefresh_(msRefresh),
            nstacks_(nstacks), 
            nchains_(nchains), 
            lengths_(nstacks*nchains),
            minEnergies_(nstacks*nchains),
            energies_(nstacks*nchains),
            nAcceptsGlobal_(nstacks*nchains),
            nSwapsGlobal_(nstacks*nchains),
            nSwapAttemptsGlobal_(nstacks*nchains),
            sigmas_(sigmas),
            acceptRates_(acceptRates),
            betas_(betas),
            swapRates_(swapRates)
    {
      std::fill(lengths_.begin(), lengths_.end(), 1);
      std::fill(minEnergies_.begin(), minEnergies_.end(), std::numeric_limits<double>::infinity());
      std::fill(nAcceptsGlobal_.begin(), nAcceptsGlobal_.end(), 1);
      std::fill(nSwapAttemptsGlobal_.begin(), nSwapAttemptsGlobal_.end(), 1); // for nan-free behaviour
    }

    void Logger::update(uint id, const State & s)
    {
      // update the variables
      lengths_[id] += 1;
      minEnergies_[id] = std::min(minEnergies_[id], s.energy);
      energies_[id] = s.energy;
      nAcceptsGlobal_[id] += s.accepted;
      nSwapsGlobal_[id] += s.swapType == SwapType::Accept;
      nSwapAttemptsGlobal_[id] += s.swapType != SwapType::NoAttempt;

      if (ch::duration_cast<ch::milliseconds>(ch::steady_clock::now() - lastPrintTime_).count() > msRefresh_)
        print();
    }

    void Logger::print()
    {
      // print to the screen
      lastPrintTime_ = ch::steady_clock::now();
      std::stringstream s;
      s << "\n\nID   Length   MinEngy     CurrEngy     Sigma   AcptRt     GlbAcptRt   Beta        SwapRt   GlbSwapRt\n";
      s << "-----------------------------------------------------------------------------------------------------\n";
      for (uint i = 0; i < lengths_.size(); i++)
      {
        if (i % nchains_ == 0 && i != 0)
          s << '\n';
        s << std::setprecision(6) << std::showpoint << i << " " << std::setw(9) << lengths_[i] << " " << std::setw(10)
          << minEnergies_[i] << " " << std::setw(10) << energies_[i] << " " << std::setw(10) << sigmas_[i](0) << " "
          << std::setw(10) << acceptRates_[i] << " " << std::setw(10) << nAcceptsGlobal_[i] / (double) lengths_[i] << " "
          << std::setw(10) << betas_[i] << " " << std::setw(10) << swapRates_[i] << " " << std::setw(10)
          << nSwapsGlobal_[i] / (double) nSwapAttemptsGlobal_[i] << " \n";
      }
      std::cout << s.str() << std::endl;
    }
  } // namespace mcmc
} // namespace stateline
