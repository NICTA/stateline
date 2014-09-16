//! Simple logger class used by the delegator
//!
//! \file logging.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \licence Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!
#pragma once

#include <chrono>
#include <vector>
#include <iostream>
#include <iomanip>
#include <glog/logging.h>
#include <Eigen/Core>

#include "infer/datatypes.hpp"

namespace ch = std::chrono;

namespace stateline
{
  namespace mcmc
  {
    class Logger
    {
      public:
        Logger(uint nstacks, uint nchains, uint msRefresh, 
            const std::vector<Eigen::VectorXd>& sigmas, const std::vector<double>& acceptRates,
            const std::vector<double>& betas, const std::vector<double>& swapRates);

        void update(uint id, const State & s);

        void print();

      private:

        ch::steady_clock::time_point lastPrintTime_;
        uint msRefresh_;
        uint nstacks_;
        uint nchains_;
        std::vector<uint> lengths_;
        std::vector<double> minEnergies_;
        std::vector<double> energies_;
        std::vector<uint> nAcceptsGlobal_;
        std::vector<uint> nSwapsGlobal_;
        std::vector<uint> nSwapAttemptsGlobal_;
        const std::vector<Eigen::VectorXd>& sigmas_;
        const std::vector<double>& acceptRates_;
        const std::vector<double>& betas_;
        const std::vector<double>& swapRates_;
    };
  } // namespace mcmc
} // namespace stateline
