//!
//! Contains the interface of an MCMC sampler.
//!
//! \file infer/sampler.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "infer/datatypes.hpp"
#include "infer/chainarray.hpp"

namespace stateline
{
  namespace mcmc
  {
    
    using ProposalFunction = std::function<Eigen::VectorXd(uint id, const Eigen::VectorXd &sample, const Eigen::VectorXd &sigma)>;
    
    Eigen::VectorXd gaussianProposal(uint id, const Eigen::VectorXd& sample,
        const Eigen::VectorXd &sigma);
    
    //! A truncated Gaussian proposal function. It randomly varies each value in
    //! the state according to a truncated Gaussian distribution. It also bounces of the
    //! walls of the hard boundaries given so as not to get stuck in corners.
    //! 
    //! \param state The current state of the chain
    //! \param sigma The standard deviation of the distribution (step size of the proposal)
    //! \param min The minimum bound of theta 
    //! \param max The maximum bound of theta 
    //! \returns The new proposed theta
    //!
    Eigen::VectorXd truncatedGaussianProposal(uint id, const Eigen::VectorXd& sample,
        const Eigen::VectorXd &sigma,
        const Eigen::VectorXd& min, const Eigen::VectorXd& max);
    
    // Assumes sigma is a flattened square covariance matrix
    Eigen::VectorXd gaussianCovProposal(uint id, const Eigen::VectorXd& sample,
        const Eigen::VectorXd &sigma);

    class Sampler
    {
      public:

        Sampler(WorkerInterface& workerInterface, 
                ChainArray& chainArray,
                const ProposalFunction& propFn,
                uint swapInterval);
        ~Sampler();
      
        std::pair<uint, State> step(const std::vector<Eigen::VectorXd>& sigmas, const std::vector<double>& betas);

        void flush();

      private:

        void propose(uint id);

        void unlock(uint id);

        WorkerInterface& workerInterface_;
        // The MCMC chain wrapper
        ChainArray& chains_;
        
        ProposalFunction propFn_;
        
        // convenience variables
        const uint nstacks_;
        const uint nchains_;

        // The proposed states in the process of being computed
        std::vector<Eigen::VectorXd> propStates_;

        // How often to attempt a swap
        uint swapInterval_;

        // How many jobs haven't been retrieved?
        uint numOutstandingJobs_;

        // Whether a chain is locked. A locked chain will wait for any outstanding
        // job results and propagate the lock.
        std::vector<bool> locked_;

        // if we haven't flushed when destructing, flush
        bool haveFlushed_;

    };
  }
}
