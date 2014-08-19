//!
//! Contains the implementation of an adaptive proposal function
//!
//! \file infer/adaptive.hpp
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <random>
#include <functional>
#include <Eigen/Core>
#include <boost/circular_buffer.hpp>

namespace stateline
{
  namespace mcmc
  {

    class SlidingWindowSigmaAdapter
    {
      public:
        SlidingWindowSigmaAdapter( uint nStacks, uint nChains, uint nDims, uint windowSize,
            double coldSigma, double sigmaFactor, uint adaptionLength, uint nStepsPerAdapt, double optimalAccept,
            double adaptRate, double minFactor, double maxFactor)
          : nStacks_(nStacks),
            nChains_(nChains),
            sigmas_(nStacks*nChains),
            acceptRates_(nStacks*nChains),
            lengths_(nStacks*nChains),
            windowSize_(windowSize),
            coldSigma_(coldSigma),
            sigmaFactor_(sigmaFactor),
            adaptionLength_(adaptionLength),
            nStepsPerAdapt_(nStepsPerAdapt),
            optimalAccept_(optimalAccept),
            adaptRate_(adaptRate),
            minFactor_(minFactor),
            maxFactor_(maxFactor)
        {
          for (uint i = 0; i < nStacks; i++)
            for (uint j = 0; j < nChains; j++)
            {
              uint id = i * nChains + j;
              double sigma = coldSigma * std::pow(sigmaFactor, j);
              sigmas_[id] = Eigen::VectorXd::Ones(nDims) * sigma;
            }

          for (uint i = 0; i < nChains * nStacks; i++)
          {
            lengths_[i] = 1; // assuming we're not recovering
            acceptRates_[i] = 1;
            acceptBuffers_.push_back(boost::circular_buffer<bool>(adaptionLength));
            acceptBuffers_[i].push_back(true); // first state always accepts
          }
        }

        void update(uint chainID, const State& state)
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
          if (lengths_[chainID] % nStepsPerAdapt_ == 0)
            adaptSigma(chainID);
        }


        std::vector<Eigen::VectorXd> sigmas() const
        {
          return sigmas_;
        }

        std::vector<double> acceptRates() const
        {
          return acceptRates_;
        }

      private:

        void adaptSigma(uint id)
        {
          double acceptRate = acceptRates_[id];
          double oldSigma= sigmas_[id](0);
          double factor = std::pow(acceptRate / optimalAccept_, adaptRate_);
          double boundFactor = std::min(std::max(factor, minFactor_), maxFactor_);
          double gamma = adaptionLength_/(double)(adaptionLength_+lengths_[id]);
          double newSigma = oldSigma * std::pow(boundFactor, gamma);
          VLOG(2) << "Adapting Sigma" << id <<":" << oldSigma << "->" << newSigma << " @acceptrate:" << acceptRate;
          // Ensure higher temperature chains have larger sigmas than chains colder than it
          if (id % nChains_ != 0 && newSigma < sigmas_[id - 1](0))
          {
            newSigma = sigmas_[id - 1](0);
          }
          sigmas_[id] = newSigma * Eigen::VectorXd::Ones(sigmas_[id].size());
        }

        uint nStacks_;
        uint nChains_;
        std::vector<boost::circular_buffer<bool>> acceptBuffers_;
        std::vector<Eigen::VectorXd> sigmas_;
        std::vector<double> acceptRates_;
        std::vector<uint> lengths_;
        uint windowSize_;
        double coldSigma_;
        double sigmaFactor_;
        uint adaptionLength_;
        uint nStepsPerAdapt_;
        double optimalAccept_;
        double adaptRate_;
        double minFactor_;
        double maxFactor_;
    };


    class SlidingWindowBetaAdapter
    {
      public:
        
        SlidingWindowBetaAdapter( uint nStacks, uint nChains, uint windowSize, double betaFactor,
            uint adaptionLength, uint nStepsPerAdapt, double optimalSwap,
              double adaptRate, double minFactor, double maxFactor)
          : nStacks_(nStacks),
            nChains_(nChains),
            betas_(nStacks*nChains),
            swapRates_(nStacks*nChains),
            lengths_(nStacks*nChains),
            windowSize_(windowSize),
            betaFactor_(betaFactor),
            adaptionLength_(adaptionLength),
            nStepsPerAdapt_(nStepsPerAdapt),
            optimalSwapRate_(optimalSwap),
            adaptRate_(adaptRate),
            minFactor_(minFactor),
            maxFactor_(maxFactor)
        {
          for (uint i = 0; i < nStacks; i++)
            for (uint j = 0; j < nChains; j++)
            {
              uint id = i * nChains + j;
              double beta = 1.0 * std::pow(betaFactor, j);
              betas_[id] = beta;
            }

          for (uint i = 0; i < nChains_ * nStacks_; i++)
          {
            swapRates_[i] = 0;
            swapBuffers_.push_back(boost::circular_buffer<bool>(adaptionLength_));
            swapBuffers_[i].push_back(false); // gets rid of a nan, not really needed
          }
        }
        
        void update(uint id, State s)
        {
          lengths_[id] += 1;
          SwapType sw = s.swapType;
          bool attempted = sw == SwapType::NoAttempt;
          if (attempted)
          {
            uint oldSize = swapBuffers_[id].size();
            double oldRate = swapRates_[id];
            bool isFull = swapBuffers_[id].full();
            bool lastSw = swapBuffers_[id].front();
            // Now push back the new state
            swapBuffers_[id].push_back(sw == SwapType::Accept);
            // Compute the new rate
            uint newSize = swapBuffers_[id].size();
            double delta = ((int)sw - (int)(lastSw&&isFull))/(double)newSize;
            double scale = oldSize/(double)newSize;
            swapRates_[id] = std::max(oldRate*scale + delta, 0.0);
          }
          if ((lengths_[id] % nStepsPerAdapt_ == 0) && (id % nChains_ != 0))
            adaptBeta(id);
        }

        std::vector<double> betas()
        {
          return betas_;
        }
        
        std::vector<double> swapRates() const
        {
          return swapRates_;
        }

      private:
        
        void adaptBeta(uint id)
        {
          // Adapt the temperature
          double swapRate = swapRates_[id];
          double rawFactor = std::pow(swapRate/optimalSwapRate_, adaptRate_);
          double boundedFactor = std::min( std::max(rawFactor, minFactor_), maxFactor_);
          double beta = betas_[id];
          double lowerBeta = betas_[id-1];// temperature changes propogate UP
          double factor = 1.0/std::max(boundedFactor, 2*beta/(beta + lowerBeta));
          // Set the temperature for this chain (because it hasn't proposed yet)
          double gamma = adaptionLength_/(double)(adaptionLength_+lengths_[id]);
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

        uint nStacks_;
        uint nChains_;
        std::vector<double> betas_;
        std::vector<double> swapRates_;
        std::vector<boost::circular_buffer<bool>> swapBuffers_;
        std::vector<uint> lengths_;
        uint windowSize_;
        double betaFactor_;
        uint adaptionLength_;
        uint nStepsPerAdapt_;
        double optimalSwapRate_;
        double adaptRate_;
        double minFactor_;
        double maxFactor_;

    };


    
    //! A function to bounce the MCMC proposal off the hard boundaries.
    //! This allows the proposal to always move around without getting stuck at
    //! 'walls'
    //! 
    //! \param val The proposed value
    //! \param min The minimum bound of theta 
    //! \param max The maximum bound of theta 
    //!  \returns The new bounced theta definitely in the bounds
    //!
    Eigen::VectorXd bouncyBounds(const Eigen::VectorXd& val,
        const Eigen::VectorXd& min, const Eigen::VectorXd& max)
    { 
      Eigen::VectorXd delta = max - min;
      Eigen::VectorXd result = val;
      Eigen::Matrix<bool, Eigen::Dynamic, 1> tooBig = (val.array() > max.array());
      Eigen::Matrix<bool, Eigen::Dynamic, 1> tooSmall = (val.array() < min.array());
      for (uint i=0; i< result.size(); i++)
      {
        bool big = tooBig(i);
        bool small = tooSmall(i);
        if (big)
        {
          double overstep = val(i)-max(i);
          int nSteps = (int)(overstep /  delta(i));
          double stillToGo = overstep - nSteps*delta(i);
          if (nSteps % 2 == 0)
            result(i) = max(i) - stillToGo;
          else
            result(i) = min(i) + stillToGo;
        }
        if (small)
        {
          double understep = min(i) - val(i);
          int nSteps = (int)(understep / delta(i));
          double stillToGo = understep - nSteps*delta(i);
          if (nSteps % 2 == 0)
            result(i) = min(i) + stillToGo;
          else
            result(i) = max(i) - stillToGo;
        }
      }
      return result;
    }
    
    //! An adaptive Gaussian proposal function. It randomly varies each value in
    //! the state according to a Gaussian distribution whose variance changes
    //! depending on the acceptance ratio of a chain. It also bounces of the
    //! walls of the hard boundaries given so as not to get stuck in corners.
    //! 
    //! \param state The current state of the chain
    //! \param sigma The standard deviation of the distribution (step size of the proposal)
    //! \param min The minimum bound of theta 
    //! \param max The maximum bound of theta 
    //! \returns The new proposed theta
    //!
    Eigen::VectorXd adaptiveGaussianProposal(uint id, const ChainArray& chains,
        const Eigen::VectorXd& min, const Eigen::VectorXd& max)
    {
      // Random number generators
      Eigen::VectorXd sigma = chains.sigma(id);
      State state = chains.lastState(id);
      static std::random_device rd;
      static std::mt19937 generator(rd());
      static std::normal_distribution<> rand; // Standard normal

      // Vary each paramater according to a Gaussian distribution
      Eigen::VectorXd proposal(state.sample.rows());
      for (int i = 0; i < proposal.rows(); i++)
        proposal(i) = state.sample(i) + rand(generator) * sigma(i);

      return bouncyBounds(proposal, min, max);
    }
  }
}
