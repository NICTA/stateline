//!
//! Contains the implementation of adaptors for sigma and beta
//!
//! \file infer/adaptive.cpp
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "infer/adaptive.hpp"

namespace stateline
{
  namespace mcmc
  {

    SlidingWindowBetaSettings SlidingWindowBetaSettings::Default()
    {
      SlidingWindowBetaSettings settings;

      settings.windowSize = 100000;
      settings.betaFactor = 1.5;
      settings.adaptionLength = 100000;
      settings.nStepsPerAdapt = 2500;
      settings.optimalSwapRate = 0.24;
      settings.adaptRate = 0.2;
      settings.minAdaptFactor = 0.8;
      settings.maxAdaptFactor = 1.25;
      return settings;
    }
    
    SlidingWindowSigmaSettings SlidingWindowSigmaSettings::Default()
    {
      SlidingWindowSigmaSettings settings;

      settings.windowSize = 100000;
      settings.coldSigma = 1.0;
      settings.sigmaFactor = 1.5;
      settings.adaptionLength = 100000;
      settings.nStepsPerAdapt = 2500;
      settings.optimalAcceptRate = 0.24;
      settings.adaptRate = 0.2;
      settings.minAdaptFactor = 0.8;
      settings.maxAdaptFactor = 1.25;
      return settings;
    }
    
    SlidingWindowSigmaAdapter::SlidingWindowSigmaAdapter( uint nStacks, uint nChains, uint nDims, 
        const SlidingWindowSigmaSettings& settings)
      : nStacks_(nStacks),
        nChains_(nChains),
        sigmas_(nStacks*nChains),
        acceptRates_(nStacks*nChains),
        lengths_(nStacks*nChains),
        s_(settings)
    {
      for (uint i = 0; i < nStacks; i++)
        for (uint j = 0; j < nChains; j++)
        {
          uint id = i * nChains + j;
          double sigma = s_.coldSigma * std::pow(s_.sigmaFactor, j);
          sigmas_[id] = Eigen::VectorXd::Ones(nDims) * sigma;
        }

      for (uint i = 0; i < nChains * nStacks; i++)
      {
        lengths_[i] = 1; // assuming we're not recovering
        acceptRates_[i] = 1;
        acceptBuffers_.push_back(boost::circular_buffer<bool>(s_.adaptionLength));
        acceptBuffers_[i].push_back(true); // first state always accepts
      }
    }

    void SlidingWindowSigmaAdapter::update(uint chainID, const State& state)
    {
      lengths_[chainID]+= 1;
      bool acc = state.accepted;
      uint oldSize = acceptBuffers_[chainID].size();
      double oldRate = acceptRates_[chainID];
      bool isFull = acceptBuffers_[chainID].full();
      bool lastAcc = acceptBuffers_[chainID].front();
      // Now push on the new state
      acceptBuffers_[chainID].push_back(acc);
      // Compute the new rate
      uint newSize = acceptBuffers_[chainID].size();
      double delta = ((int)acc - (int)(lastAcc&&isFull))/(double)newSize;
      double scale = oldSize/(double)newSize;
      acceptRates_[chainID] = std::max(oldRate*scale + delta, 0.0);
      if (lengths_[chainID] % s_.nStepsPerAdapt == 0)
        adaptSigma(chainID);
    }

    const std::vector<Eigen::VectorXd> &SlidingWindowSigmaAdapter::sigmas() const
    {
      return sigmas_;
    }

    const std::vector<double> &SlidingWindowSigmaAdapter::acceptRates() const
    {
      return acceptRates_;
    }


    void SlidingWindowSigmaAdapter::adaptSigma(uint id)
    {
      double acceptRate = acceptRates_[id];
      double oldSigma= sigmas_[id](0);
      double factor = std::pow(acceptRate / s_.optimalAcceptRate, s_.adaptRate);
      double boundFactor = std::min(std::max(factor, s_.minAdaptFactor), s_.maxAdaptFactor);
      double gamma = s_.adaptionLength/(double)(s_.adaptionLength+lengths_[id]);
      double newSigma = oldSigma * std::pow(boundFactor, gamma);
      VLOG(2) << "Adapting Sigma" << id <<":" << oldSigma << "->" << newSigma << " @acceptrate:" << acceptRate;
      // Ensure higher temperature chains have larger sigmas than chains colder than it
      if (id % nChains_ != 0 && newSigma < sigmas_[id - 1](0))
      {
        newSigma = sigmas_[id - 1](0);
      }
      sigmas_[id] = newSigma * Eigen::VectorXd::Ones(sigmas_[id].size());
    }



    SlidingWindowBetaAdapter::SlidingWindowBetaAdapter( uint nStacks, uint nChains, 
        const SlidingWindowBetaSettings& settings)
      : nStacks_(nStacks),
      nChains_(nChains),
      betas_(nStacks*nChains),
      swapRates_(nStacks*nChains),
      lengths_(nStacks*nChains),
      s_(settings)
    {
      for (uint i = 0; i < nStacks; i++)
        for (uint j = 0; j < nChains; j++)
        {
          uint id = i * nChains + j;
          double beta = 1.0 * std::pow(s_.betaFactor, j);
          betas_[id] = beta;
        }

      for (uint i = 0; i < nChains_ * nStacks_; i++)
      {
        swapRates_[i] = 0.0;
        swapBuffers_.push_back(boost::circular_buffer<bool>(s_.adaptionLength));
        swapBuffers_[i].push_back(false); // gets rid of a nan, not really needed
      }
    }

    void SlidingWindowBetaAdapter::update(uint id, State s)
    {
      lengths_[id] += 1;
      bool attempted = s.swapType != SwapType::NoAttempt;
      if (attempted)
      {
        bool sw = s.swapType == SwapType::Accept;
        uint oldSize = swapBuffers_[id].size();
        double oldRate = swapRates_[id];
        bool isFull = swapBuffers_[id].full();
        bool lastSw = swapBuffers_[id].front();
        // Now push back the new state
        swapBuffers_[id].push_back(sw);
        // Compute the new rate
        uint newSize = swapBuffers_[id].size();
        double delta = ((int)sw - (int)(lastSw&&isFull))/(double)newSize;
        double scale = oldSize/(double)newSize;
        swapRates_[id] = std::max(oldRate*scale + delta, 0.0);
      }
      // Every so often adapt beta
      if ((lengths_[id] % s_.nStepsPerAdapt == 0) && (id % nChains_ != 0))
        adaptBeta(id);
    }

    const std::vector<double> &SlidingWindowBetaAdapter::betas() const
    {
      return betas_;
    }

    const std::vector<double> &SlidingWindowBetaAdapter::swapRates() const
    {
      return swapRates_;
    }

    void SlidingWindowBetaAdapter::adaptBeta(uint id)
    {
      // Adapt the temperature
      double swapRate = swapRates_[id-1]; 
      double rawFactor = std::pow(swapRate/s_.optimalSwapRate, s_.adaptRate);
      double boundedFactor = std::min( std::max(rawFactor, s_.minAdaptFactor), s_.maxAdaptFactor);
      double beta = betas_[id];
      double lowerBeta = betas_[id-1];// temperature changes propogate UP
      double factor = 1.0/std::max(boundedFactor, 2*beta/(beta + lowerBeta));
      // Set the temperature for this chain (because it hasn't proposed yet)
      double gamma = s_.adaptionLength/(double)(s_.adaptionLength+lengths_[id]);
      double newbeta = betas_[id] * std::pow(factor, gamma);
      betas_[id] = newbeta;
      // Just for logging
      VLOG(2) << "Adapting Beta" << id << ":" << beta << "->" << newbeta << " @swaprate:" << swapRate;
      // Loop through the other temperatures
      uint coldestChainId = (uint)(id / (double)nChains_) * nChains_;
      uint hottestChainId = coldestChainId + nChains_-1;
      // Set the next temperatures for the other chains (as they're busy)
      for (uint i = id+1; i <= hottestChainId; i++)
      {
        betas_[i] = betas_[i] * std::pow(factor, gamma);
      }
    }

    SigmaCovarianceAdapter::SigmaCovarianceAdapter(uint nStacks, uint nChains, uint nDims,
        const SlidingWindowSigmaSettings &settings)
      : adapter_(nStacks, nChains, nDims, settings),
      lengths_(nStacks * nChains, 0),
      covs_(nStacks * nChains, Eigen::MatrixXd::Identity(nDims, nDims)),
      a_(nStacks * nChains, Eigen::MatrixXd::Zero(nDims, nDims)),
      u_(nStacks * nChains, Eigen::VectorXd::Zero(nDims)),
      sigmas_(nStacks * nChains)
    {
      for (uint i = 0; i < sigmas_.size(); i++)
      {
        sigmas_[i] = adapter_.sigmas()[i][0] *
          Eigen::Map<Eigen::VectorXd>(covs_[i].data(), covs_[i].size());
      }
    }

    void SigmaCovarianceAdapter::update(uint i, const State &s)
    {
      adapter_.update(i, s);

      double n = (double)lengths_[i];
      Eigen::VectorXd x = s.sample;

      a_[i] = a_[i] * (n / (n + 1)) + (x * x.transpose()) / (n + 1);
      u_[i] = u_[i] * (n / (n + 1)) + x / (n + 1);

      covs_[i] = a_[i] - (u_[i] * u_[i].transpose());// / (n + 1);

      // Flatten the cov matrix to get a sigma vector
      sigmas_[i] = adapter_.sigmas()[i][0] *
        Eigen::Map<Eigen::VectorXd>(covs_[i].data(), covs_[i].size());

      lengths_[i]++;
    }

    const std::vector<double> &SigmaCovarianceAdapter::acceptRates() const
    {
      return adapter_.acceptRates();
    }

    const std::vector<Eigen::VectorXd> &SigmaCovarianceAdapter::sigmas() const
    {
      return sigmas_;
    }

    const std::vector<Eigen::MatrixXd> &SigmaCovarianceAdapter::covs() const
    {
      return covs_;
    }

  } // mcmc
} // stateline
