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

        void update(uint chainID, double sigm, double t, bool accepted);
        void betaUpdate(uint chainID, double bl, double bh, bool acc);

        double predict(uint chainID, double t) const;
        double computeSigma(uint chainID, double t);
        void computeBetaStack(uint chainID);

        const std::vector<double>& rates() const;
        const std::vector<double>& values() const;

      private:

        uint nStacks_;
        uint nTemps_;
        double optimalRate_;

        // For estimating proposal lenghts (watch out for alloc in Vector2,4)
        std::vector<Eigen::Vector3d> mu_xy_;
        std::vector<Eigen::Matrix3d> mu_xx_;
        std::vector<Eigen::Vector3d> weight_;
        std::vector<double> count_;

        // For estimating the accept rates using a rolling window
        std::vector<std::deque<int>> window_;
        std::vector<int> window_sum_;
        std::vector<double> rates_;
        std::vector<double> values_;
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
