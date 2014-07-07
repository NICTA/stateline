//!
//! Contains the interface for the multivariate Gaussian distribution.
//!
//! \file distrib/multigaussian.hpp
//! \author Alistair Reid
//! \date April 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <cmath>
#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/Dense>
#include <Eigen/Cholesky>

namespace obsidian
{
  namespace distrib
  {
    //! Represents a multivariate Gaussian distribution.
    //!
    struct MultiGaussian
    {
      //! Create a multivariate Gaussian distribution.
      //!
      //! \param mu The means of each dimension of the distribution.
      //! \param sigma The covariance matrix.
      //!
      MultiGaussian(const Eigen::VectorXd& mu, const Eigen::MatrixXd& sigma);

      //! Create a multivariate Gaussian distribution.
      //!
      //! \param mu The means of each dimension of the distribution.
      //! \param sigma The covariance matrix.
      //! \param w, h The shape of the distribution.
      //!
      MultiGaussian(const Eigen::VectorXd& mu, const Eigen::MatrixXd& sigma, int w, int h);

      //! The means of each dimension.
      Eigen::VectorXd mu;
      Eigen::MatrixXd sigLInv;
      Eigen::MatrixXd sigL;

      //! The covariance matrix.
      Eigen::MatrixXd sigma;

      //! The shape of the distribution.
      std::pair<uint, uint> shape;
    };

    //! 
    MultiGaussian coupledGaussianBlock(const Eigen::MatrixXd& mean, double coupledSD, double decoupledSD);

    //! Compute the log PDF of a multivariate Gaussian distribution
    double logPDF(const Eigen::MatrixXd& theta, const MultiGaussian& input, const Eigen::MatrixXd& thetaMin,
                  const Eigen::MatrixXd& thetaMax);

    //! Compute the log PDF of a multivariate Gaussian distribution
    double logPDF(const Eigen::VectorXd& theta, const MultiGaussian& input, const Eigen::VectorXd& thetaMin,
                  const Eigen::VectorXd& thetaMax);

    double uniformLogPDF(const Eigen::MatrixXd& theta, const MultiGaussian& input, const Eigen::MatrixXd& thetaMins,
                         const Eigen::MatrixXd& thetaMaxs);

    //! Draw a sample from a multivariate Gaussian distribution.
    //
    Eigen::MatrixXd drawValues(const MultiGaussian &input, std::mt19937 &gen);

    //! Draw a sample from a multivariate Gaussian distribution.
    //
    std::vector<Eigen::MatrixXd> drawFrom(const std::vector<distrib::MultiGaussian>& prior, std::mt19937& gen,
                                          const std::vector<Eigen::MatrixXd>& mins, const std::vector<Eigen::MatrixXd>& maxs,
                                          const std::vector<bool>& uniformFlags);

    //! Draw a sample from a multivariate Gaussian distribution.
    //
    std::vector<Eigen::VectorXd> drawVectorFrom(const std::vector<distrib::MultiGaussian>& prior, std::mt19937& gen,
                                                const std::vector<Eigen::VectorXd>& mins, const std::vector<Eigen::VectorXd>& maxs);

  }
}
