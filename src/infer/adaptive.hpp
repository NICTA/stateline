//!
//! Contains the implementation of adaption classes for sigma and beta
//!
//! \file infer/adaptive.hpp
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <Eigen/Core>
#include <boost/circular_buffer.hpp>
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

      static SlidingWindowSigmaSettings Default();
    
    };

    class SlidingWindowSigmaAdapter
    {
      public:
        SlidingWindowSigmaAdapter( uint nStacks, uint nChains, uint nDims, 
            const SlidingWindowSigmaSettings& settings);

        void update(uint chainID, const State& state);

        const std::vector<Eigen::VectorXd> &sigmas() const;

        const std::vector<double> &acceptRates() const;

      private:

        void adaptSigma(uint id);

        uint nStacks_;
        uint nChains_;
        std::vector<boost::circular_buffer<bool>> acceptBuffers_;
        std::vector<Eigen::VectorXd> sigmas_;
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

      static SlidingWindowBetaSettings Default();

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

        uint nStacks_;
        uint nChains_;
        std::vector<double> betas_;
        std::vector<double> swapRates_;
        std::vector<boost::circular_buffer<bool>> swapBuffers_;
        std::vector<uint> lengths_;
        SlidingWindowBetaSettings s_;
    };


    class SigmaCovarianceAdapter
    {
      public:
        SigmaCovarianceAdapter(uint nStacks, uint nChains, uint nDims,
            const SlidingWindowSigmaSettings &settings);

        void update(uint i, const State &s);

        const std::vector<double> &acceptRates() const;

        const std::vector<Eigen::VectorXd> &sigmas() const;

        const std::vector<Eigen::MatrixXd> &covs() const;

      private:
        SlidingWindowSigmaAdapter adapter_;
        std::vector<uint> lengths_;
        std::vector<Eigen::MatrixXd> covs_;
        std::vector<Eigen::MatrixXd> a_;
        std::vector<Eigen::VectorXd> u_;
        std::vector<Eigen::VectorXd> sigmas_;
    };

    
    class BlockSigmaAdapter
    {
      public:
        BlockSigmaAdapter(uint nStacks, uint nChains, uint nDims,
            const std::vector<uint> &blocks,
            const SlidingWindowSigmaSettings &settings)
          : blocks_(blocks.size()),
            curBlocks_(nStacks * nChains),
            maskedSigmas_(nStacks * nChains)
        {
          // We will re-number all the blocks, so that blocks are number from
          // 0 to N contiguously.
          std::map<uint, uint> used;
          
          for (uint i = 0; i < nDims; i++)
          {
            if (!used.count(blocks[i]))
            {
              // Encountered a new block number
              used.insert(std::make_pair(blocks[i], used.size()));

              // Create an adapter just for this block
              adapters_.push_back(SlidingWindowSigmaAdapter(
                    nStacks, nChains, nDims, settings));
            }

            blocks_(i) = used[blocks[i]];
          }

          for (uint i = 0; i < maskedSigmas_.size(); i++)
          {
            curBlocks_[i] = 0;

            maskedSigmas_[i] = (blocks_ == curBlocks_[i]).cast<double>() *
              adapters_[0].sigmas()[i].array();
          }

          numBlocks_ = used.size();
        }

        void update(uint i, const State &s)
        {
          adapters_[curBlocks_[i]].update(i, s);

          // Change the mask to the next block
          curBlocks_[i] = (curBlocks_[i] + 1) % numBlocks_;

          // Mask out sigmas not in the current block
          maskedSigmas_[i] = (blocks_ == curBlocks_[i]).cast<double>() *
            adapters_[curBlocks_[i]].sigmas()[i].array();
        }

        const std::vector<double> &acceptRates() const
        {
          return adapters_[0].acceptRates();
        }

        const std::vector<Eigen::VectorXd> &sigmas() const
        {
          return maskedSigmas_;
        }

      private:
        std::vector<SlidingWindowSigmaAdapter> adapters_;
        Eigen::ArrayXd blocks_;
        std::vector<uint> curBlocks_;
        std::vector<Eigen::VectorXd> maskedSigmas_;
        uint numBlocks_;
    };
    
  }
}
