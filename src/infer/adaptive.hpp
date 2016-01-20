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

        double pick(uint chainID, double t) const

        const std::vector<double> &rates() const;

      private:
        const double min_logsigma_ = -8;
        const double max_logsigma_ = 3;
        const int n_window_ = 1000;

        uint nStacks_;
        uint nTemps_;
        
        double optimalRate_;

        // For estimating proposal lenghts
        // NOTE: if these were 2 and not 3, fixed-size measures required
        // http://eigen.tuxfamily.org/dox-devel/group__TopicFixedSizeVectorizable.html
        std::vector<Eigen::VectorXd> mu_xy_;
        std::vector<Eigen::VectorXd> weight_;
        std::vector<Eigen::MatrixXd> mu_xx_;
        std::vector<double> count_;

        // For estimating accept rates
        std::vector<std::deque<int>> window_;
        std::vector<int> window_sum_;
        std::vector<double> rates_;
        std::vector<double> estimates_;
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
