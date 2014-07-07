//!
//! Contains the implementation of functions for computing the likelihood.
//!
//! \file likelihood/likelihood.hpp
//! \author Darren Shen
//! \date May 2014
//! \license General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <Eigen/Dense>

#include "datatype/datatypes.hpp"

namespace obsidian
{
  namespace lh
  {
    //! Calculate the Gaussian log likelihood.
    //! 
    //! \param real Vector containing the real sensor data.
    //! \param candidate Vector containing the simulated sensor data.
    //! \param sensorSd The standard deviation of sensor noise.
    //!
    double gaussian(const Eigen::VectorXd &real, const Eigen::VectorXd &candidate, double sensorSd);

    //!
    //! Calculate the normal inverse Gamma marginal log likelihood
    //! 
    //! \param real Vector containing the real sensor data
    //! \param candidate Vector containing the simulated sensor data
    //! \param A, B Alpha and beta parameters
    //!
    double normalInverseGamma(const Eigen::VectorXd &real, const Eigen::VectorXd &candidate, double A, double B);

    template<ForwardModel f>
    double likelihood(const typename Types<f>::Results& synthetic, const typename Types<f>::Results& real,
                      const typename Types<f>::Spec& spec);

    Eigen::VectorXd mtLikelihoodVector(const Eigen::MatrixX4cd& impedences);

    std::vector<double> likelihoodAll(const GlobalResults& synthetic, const GlobalResults& real, const GlobalSpec& spec,
                                      const std::set<ForwardModel>& enabled);
  }
}
