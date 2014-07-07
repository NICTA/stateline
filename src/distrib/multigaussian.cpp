//!
//! Contains the implementation of the multivariate Gaussian distribution.
//!
//! \file distrib/multigaussian.cpp
//! \author Alistair Reid
//! \date April 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "distrib/multigaussian.hpp"
#include "glog/logging.h"

namespace obsidian
{
  namespace distrib
  {
    MultiGaussian::MultiGaussian(const Eigen::VectorXd& mu, const Eigen::MatrixXd& sigma)
        : mu(mu), sigma(sigma)
    {
      shape.first = mu.rows();
      shape.second = mu.cols();
      sigL = sigma.llt().matrixL();
      sigLInv = sigL.lu().solve(Eigen::MatrixXd::Identity(sigma.rows(), sigma.rows()));
    }

    MultiGaussian::MultiGaussian(const Eigen::VectorXd& mu, const Eigen::MatrixXd& sigma, int w, int h)
        : mu(mu), sigma(sigma), shape(w, h)
    {
      sigL = sigma.llt().matrixL();
      sigLInv = sigL.lu().solve(Eigen::MatrixXd::Identity(sigma.rows(), sigma.rows()));
    }

    // multivariate block but with a specified mean for all points
    MultiGaussian coupledGaussianBlock(const Eigen::MatrixXd& mean, double coupledSD, double decoupledSD)
    {
      uint nX = mean.rows();
      uint nY = mean.cols();
      uint nParams = nX * nY;
      Eigen::MatrixXd mu = mean;
      mu.resize(nParams, 1);
      Eigen::MatrixXd cov = (coupledSD * coupledSD) * Eigen::MatrixXd::Ones(nParams, nParams)
          + (decoupledSD * decoupledSD) * Eigen::MatrixXd::Identity(nParams, nParams);
      return MultiGaussian(mu, cov, nX, nY);
    }

    double logPDF(const Eigen::MatrixXd& theta, const MultiGaussian& input, const Eigen::MatrixXd& thetaMins,
                  const Eigen::MatrixXd& thetaMaxs)
    {
      Eigen::VectorXd thetaV = Eigen::VectorXd::Map(theta.data(), theta.rows() * theta.cols());
      Eigen::VectorXd thetaMinV = Eigen::VectorXd::Map(thetaMins.data(), thetaMins.rows() * thetaMins.cols());
      Eigen::VectorXd thetaMaxV = Eigen::VectorXd::Map(thetaMaxs.data(), thetaMaxs.rows() * thetaMaxs.cols());
      return logPDF(thetaV, input, thetaMinV, thetaMaxV);
    }

    double logPDF(const Eigen::VectorXd& theta, const MultiGaussian& input, const Eigen::VectorXd& thetaMins,
                  const Eigen::VectorXd& thetaMaxs)
    {
      bool failLowBound = (theta.array() < thetaMins.array()).any();
      bool failHighBound = (theta.array() > thetaMaxs.array()).any();
      if (failLowBound || failHighBound)
        return -std::numeric_limits<double_t>::infinity();
      constexpr double log2PI = 1.837877066409345;
      Eigen::VectorXd X = theta - input.mu;
      double logDetSig = -2 * input.sigLInv.diagonal().array().log().sum();
      // The following line seems weird but we dont want to solve sigLInv*sigLInv' ...
      Eigen::ArrayXd fitTerm = (input.sigLInv * X).array();
      double dataFit = fitTerm.pow(2.0).sum();
      double norm = theta.size() * log2PI;
      return -0.5 * (logDetSig + dataFit + norm);
    }

    double uniformLogPDF(const Eigen::MatrixXd& theta, const MultiGaussian& input, const Eigen::MatrixXd& thetaMins,
                         const Eigen::MatrixXd& thetaMaxs)
    {
      bool failLowBound = (theta.array() < thetaMins.array()).any();
      bool failHighBound = (theta.array() > thetaMaxs.array()).any();
      if (failLowBound || failHighBound)
      {
        return -std::numeric_limits<double_t>::infinity();
      } else
      {
        return -std::log((thetaMaxs - thetaMins).array().prod());
      }
    }

    Eigen::MatrixXd drawValues(const MultiGaussian &input, std::mt19937 &gen)
    {
      // populate a vector with uniform randomsi
      std::normal_distribution<double> dist(0, 1);
      uint nDims = input.shape.first * input.shape.second;
      CHECK_EQ(nDims, input.mu.size());
      Eigen::VectorXd randnDraws(nDims);
      for (uint i = 0; i < nDims; i++)
        randnDraws(i) = dist(gen);
      Eigen::VectorXd means(nDims);
      means = input.mu;
      // Mu and nDraws are n*1 vectors
      Eigen::MatrixXd output = means + input.sigL * randnDraws;
      output.resize(input.shape.first, input.shape.second); // keeps the old memory unless the number of elements changes
      return output;
    }

    Eigen::MatrixXd drawUniformValues(const Eigen::MatrixXd& min, const Eigen::MatrixXd& max, std::mt19937 &gen)
    {
      // populate a vector with uniform randomsi
      std::uniform_real_distribution<double> dist(0, 1);
      Eigen::MatrixXd randnDraws(min.rows(), min.cols());
      for (uint i = 0; i < min.rows(); i++)
      {
        for (uint j = 0; j < min.cols(); j++)
        {
          randnDraws(i, j) = dist(gen);
        }
      }
      // Mu and nDraws are n*1 vectors
      Eigen::MatrixXd output = min + Eigen::MatrixXd((max - min).array() * (randnDraws.array()));
      return output;
    }

    std::vector<Eigen::MatrixXd> drawFrom(const std::vector<distrib::MultiGaussian>& prior, std::mt19937& gen,
                                          const std::vector<Eigen::MatrixXd>& mins, const std::vector<Eigen::MatrixXd>& maxs,
                                          const std::vector<bool>& uniformFlags)
    {
      uint nElements = prior.size();
      std::vector<Eigen::MatrixXd> drawVals;
      for (uint i = 0; i < nElements; i++)
      {
        Eigen::MatrixXd val;
        bool found = false;
        bool isUniform = uniformFlags[i];
        uint c = 0;
        if (isUniform)
        {
          val = drawUniformValues(mins[i], maxs[i], gen);
          found = true;
        } else
        {
          while (!found && (c < 100000))
          {
            val = drawValues(prior[i], gen);
            bool failLowBound = (val.array() < mins[i].array()).any();
            bool failHighBound = (val.array() > maxs[i].array()).any();
            found = (!failLowBound) && (!failHighBound);
            c++;
          }
          if (!found)
            LOG(FATAL)<< "Could not find a valid prior sample given the hard bounds.";
          }
        drawVals.push_back(val);
      }
      return drawVals;
    }

    std::vector<Eigen::VectorXd> drawVectorFrom(const std::vector<distrib::MultiGaussian>& prior, std::mt19937& gen,
                                                const std::vector<Eigen::VectorXd>& mins, const std::vector<Eigen::VectorXd>& maxs)
    {
      uint nElements = prior.size();
      std::vector<Eigen::VectorXd> drawVals;
      for (uint i = 0; i < nElements; i++)
      {
        Eigen::VectorXd val;
        bool found = false;
        uint c = 0;
        while (!found && (c < 100000))
        {
          val = drawValues(prior[i], gen);
          bool failLowBound = (val.array() < mins[i].array()).any();
          bool failHighBound = (val.array() > maxs[i].array()).any();
          found = (!failLowBound) && (!failHighBound);
          c++;
        }
        if (!found)
        {
          LOG(FATAL)<< "Could not find a valid prior sample given the hard bounds.";
        }
        drawVals.push_back(val);
      }
      return drawVals;
    }

  } // namespace distrib
} // namespace obsidian
