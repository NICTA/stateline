//!
//! Contains the implementation of adaption classes for sigma and beta
//!
//! \file infer/adaptive.hpp
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <Eigen/Core>
#include <boost/circular_buffer.hpp>
#include <json.hpp>
#include "../infer/datatypes.hpp"

namespace stateline
{
  namespace mcmc
  {
    
    class RegressionAdapter
    {
      public:
        RegressionAdapter( uint nStacks, uint nTemps, double optimalRate);

        void update(uint chainID, const State& state);

        const std::vector<double> &estimates() const;

        const std::vector<double> &rates() const;
      private:
        uint nStacks_;
        uint nTemps_;
        double optimalRate_;
        std::vector<double> estimates_;
        std::vector<double> rates_;
    };

    class CovarianceEstimator
    {
      public:
        CovarianceEstimator(uint nStacks, uint nTemps, uint nDims);
        void update(uint i, const Eigen::VectorXd& sample);
        const std::vector<Eigen::MatrixXd> &covariances() const;

      private:
        std::vector<uint> lengths_;
        std::vector<Eigen::MatrixXd> covs_;
        std::vector<Eigen::MatrixXd> a_;
        std::vector<Eigen::VectorXd> u_;
    };
    
  }
}
