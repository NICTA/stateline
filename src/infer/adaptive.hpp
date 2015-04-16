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
#include "infer/datatypes.hpp"

namespace stateline
{
  namespace mcmc
  {
    struct SlidingWindowSigmaSettings
    {
      //! The number of states in the sliding window used to calculate the
      //! acceptance rate
      uint windowSize;

      //! The value of sigma for the coldest (beta=1) chain
      double coldSigma;

      //! The geometric factor for each hotter chain's sigma
      double sigmaFactor;

      //! The time constant of the diminishing adaption
      uint adaptionLength;

      //! The number of steps between adaptions
      uint nStepsPerAdapt;

      //! The accept rate being targeted by the adaption of sigma
      double optimalAcceptRate;

      //! The multiplicative rate at which adaption occurs
      double adaptRate;

      //! The minimum multiplicative factor by which sigma can change in a
      //! single adaption
      double minAdaptFactor;

      //! The maximum multiplicative factor by which sigma can change in a
      //! single adaption
      double maxAdaptFactor;

      SlidingWindowSigmaSettings(const nlohmann::json& config)
      {
        // TODO: defaults
        auto sigmaConfig = config["sigmaAdaption"];
        windowSize = sigmaConfig["windowSize"];
        coldSigma = config["sigma"];
        sigmaFactor = config["parallelTempering"]["sigmaFactor"];
        adaptionLength = sigmaConfig["adaptionLength"];
        nStepsPerAdapt = sigmaConfig["stepsPerAdapt"];
        optimalAcceptRate = sigmaConfig["optimalAcceptRate"];
        adaptRate = sigmaConfig["adaptRate"];
        minAdaptFactor = sigmaConfig["adaptFactor"]["min"];
        maxAdaptFactor = sigmaConfig["adaptFactor"]["max"];
      }
    };

    class SlidingWindowSigmaAdapter
    {
      public:
        SlidingWindowSigmaAdapter( uint nStacks, uint nChains, uint nDims, 
            const SlidingWindowSigmaSettings& settings);

        void update(uint chainID, const State& state);

        const std::vector<double> &sigmas() const;

        const std::vector<double> &acceptRates() const;

      private:
        void adaptSigma(uint id);

        uint nChains_;
        std::vector<boost::circular_buffer<bool>> acceptBuffers_;
        std::vector<double> sigmas_;
        std::vector<double> acceptRates_;
        std::vector<uint> lengths_;
        SlidingWindowSigmaSettings s_;
    };

    struct SlidingWindowBetaSettings
    {
      //! The number of states in the sliding window used to calculate the
      //! swap rate
      uint windowSize;

      //! The geometric factor for each hotter chain's sigma
      double betaFactor;

      //! The time constant of the diminishing adaption
      uint adaptionLength;

      //! The number of steps between adaptions
      uint nStepsPerAdapt;

      //! The swap rate being targeted by the adaption of beta
      double optimalSwapRate;

      //! The multiplicative rate at which adaption occurs
      double adaptRate;

      //! The minimum multiplicative factor by which beta can change in a
      //! single adaption
      double minAdaptFactor;

      //! The maximum multiplicative factor by which beta can change in a
      //! single adaption
      double maxAdaptFactor;

      SlidingWindowBetaSettings(const nlohmann::json& config)
      {
        // TODO: defaults
        auto betaConfig = config["parallelTempering"]["betaAdaption"];
        windowSize = betaConfig["windowSize"];
        betaFactor = config["parallelTempering"]["betaFactor"];
        adaptionLength = betaConfig["adaptionLength"];
        nStepsPerAdapt = betaConfig["stepsPerAdapt"];
        optimalSwapRate = betaConfig["optimalSwapRate"];
        adaptRate = betaConfig["adaptRate"];
        minAdaptFactor = betaConfig["adaptFactor"]["min"];
        maxAdaptFactor = betaConfig["adaptFactor"]["max"];
      }
    };

    class SlidingWindowBetaAdapter
    {
      public:
        SlidingWindowBetaAdapter(uint nStacks, uint nChains, 
            const SlidingWindowBetaSettings& settings);
        
        void update(uint id, State s);

        const std::vector<double> &betas() const;
        
        const std::vector<double> &swapRates() const;

      private:
        
        void adaptBeta(uint id);

        uint nChains_;
        std::vector<double> betas_;
        std::vector<double> swapRates_;
        std::vector<boost::circular_buffer<bool>> swapBuffers_;
        std::vector<uint> lengths_;
        SlidingWindowBetaSettings s_;
    };


    class CovarianceEstimator
    {
      public:
        CovarianceEstimator(uint nStacks, uint nChains, uint nDims);
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
