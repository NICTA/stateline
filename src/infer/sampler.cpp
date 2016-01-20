//!
//! Contains the implementation of an MCMC sampler.
//!
//! \file infer/sampler.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "infer/sampler.hpp"
#include <functional>
#include <iostream>

namespace ph = std::placeholders;

namespace stateline
{
  namespace mcmc
  {
    // A function to bounce the MCMC proposal off the hard boundaries.
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

    GaussianCovProposal::GaussianCovProposal(uint nStacks, uint nChains, uint
            nDims, const ProposalBounds& bounds)
      : gen_(std::random_device()()), sigL_(nStacks * nChains),
      bounds_(bounds), covarianceEstimator_(nStacks, nChains, nDims)
    {

      for (uint i = 0; i < nStacks * nChains; i++)
        sigL_[id] = Eigen::MatrixXd::Identity(nDims, nDims); 

      if ((bounds_.min.rows() == nDims) && (bounds_.max.rows() == nDims))
      {
        LOG(INFO) << "Using a bounded Gaussian proposal function";
        proposeFn_ = std::bind(&GaussianCovProposal::boundedPropose, this,
                ph::_1, ph::_2, ph::_3);
      }
      else
      {
        LOG(INFO) << "Using a Gaussian proposal function";
        proposeFn_ = std::bind(&GaussianCovProposal::propose, this, ph::_1,
                ph::_2, ph::_3);
      }
    }

    Eigen::VectorXd GaussianCovProposal::propose(uint id, const Eigen::VectorXd &sample, double sigma)
    {
      uint n = sample.size();

      Eigen::VectorXd randn(n);
      for (uint i = 0; i < n; i++)
        randn(i) = rand_(gen_);

      return sample + sigL_[id] * randn * sigma;
    }

    Eigen::VectorXd GaussianCovProposal::boundedPropose(uint id, const Eigen::VectorXd& sample, double sigma)
    {
      return bouncyBounds(propose(id, sample, sigma), bounds_.min, bounds_.max);
    }

    Eigen::VectorXd GaussianCovProposal::operator()(uint id, const Eigen::VectorXd &sample, double sigma)
    {
      return proposeFn_(id, sample, sigma);
    }

    void GaussianCovProposal::update(uint id, const Eigen::VectorXd &sample)
    {
      covEstimator.update(id, sample);
      Eigen::MatrixXd cov = covEstimator.covariances()[id];
      sigL_[id] = cov.llt().matrixL();
    }

    Sampler::Sampler(comms::Requester& requester, 
                     std::vector<uint> jobTypes,
                     ChainArray& chainArray,
                     const ProposalFunction& propFn,
                     const RegressionAdapter& sigmaAdapter,
                     const RegressionAdapter& betaAdapter,
                     uint swapInterval)
      : requester_(requester),
        jobTypes_(std::move(jobTypes)),
        chains_(chainArray),
        propFn_(propFn),
        sigmaAdapter_(sigmaAdapter),
        betaAdapter_(betaAdapter),
        nstacks_(chains_.numStacks()),
        nchains_(chains_.numTemps()),
        propStates_(nstacks_*nchains_),
        swapInterval_(swapInterval),
        numOutstandingJobs_(0),
        locked_(nstacks_ * nchains_, false),
        haveFlushed_(true)
    {
      // Start all the chains from hottest to coldest
      for (uint i = 0; i < chains_.numTotalChains(); i++)
      {
        uint c = chains_.numTotalChains() - i - 1;
        propose(c);
      }
    }

    Sampler::~Sampler()
    {
      if (haveFlushed_ == false)
        flush();
    }
    
    std::pair<uint, State> Sampler::step()
    {

      haveFlushed_ = false;

      // Listen for replies. As soon as a new state comes back,
      // add it to the corresponding chain, and submit a new proposed state
      auto result = requester_.retrieve();  // id, likelihood factors
      uint id = result.first;
      double energy = 0.0;
      for (const auto& r : result.second)
      {
        energy += r;
      }
      numOutstandingJobs_--;

      // Retrieve the sigma and beta values of this chain:
      double this_sigma = sigmaAdapter_.sigmas()[id];  // historical sigma
      double this_beta = betaAdapter_.betas()[id];  // historical beta

      // TODO(Al): make sigma and beta arguments to .append rather than part
      //           of chain state - all this block should be one call.
      // Esp considering the adapters trace their last use...
      chains_.setSigma(id, this_sigma);
      chains_.setBeta(id, this_beta);
      chains_.append(id, propStates_[id], energy);
      State state = chains_.lastState(id); 
      bool accepted = state.accepted

      sigmaAdapter_.update(id, this_sigma, 1./this_beta, state.accepted);
      proposal_.update(id, state.sample);  // update sample covariance
      // the new sigma is generated in propose

      if (locked_[id])
      {
        // If this chain was locked, it means that the chain above (hotter)
        // locked it and is waiting for swap.
        bool swapped = chains_.swap(id, id + 1) == SwapType::Accept;  // TODO(Al)...

        unlock(id);  // Proposes for id+1 (getting it moving again) and 
                     // Propagates the lock to id-1 ready for this swap
                     // Handles getting proposals back into system


        // AL UPTOHERE!
        // we can set beta any time
        // Beta - consider swapping coldest to hottest?
        // Constrained update the predictor? or batch update
        // Could make it that each time it sets beta to the min of one above
        // and target value... that would make sense, and allow it to continue
        // to grow.
        betaAdapter_.betaUpdate(id,  state.swapType==SwapType.Accept);
        
        // Assign a new beta that is not more than the temperature of the
        // parent chain

      }
      else if (chains_.isHottestInStack(id) && chains_.length(id) % swapInterval_ == 0 && chains_.numTemps() > 1)
      {
        // The hottest chain is ready to swap. 
        locked_[id - 1] = true;
      }
      else
      {
        propose(id);  // computes a new sigma
      }
          
            
      return {id, chains_.lastState(id)};
    }


    void Sampler::propose(uint id)
    {
      double sigma = sigmaAdapter_.sigma(id);
      propStates_[id] = propFn_(id, chains_.lastState(id).sample, sigma);
      requester_.submit(id, jobTypes_, propStates_[id]);
      numOutstandingJobs_++;
    }


    void Sampler::flush()
    {
      haveFlushed_ = true;
      // Retrieve all outstanding job results.
      while (numOutstandingJobs_--)
      {
        auto result = requester_.retrieve();
        uint id = result.first;
        double energy = 0.0;
        for (const auto& r : result.second)
        {
          energy += r;
        }
        chains_.append(id, propStates_[id], energy);
      }

      // Manually flush any chain states that are in memory to disk
      for (uint i = 0; i < chains_.numTotalChains(); i++)
        chains_.flushToDisk(i);
    }

    void Sampler::unlock(uint id)
    {
      // Unlock this chain
      locked_[id] = false;

      // The hotter chain no longer has to wait for this chain, so
      // it can propose new state
      propose(id + 1);

      // Check if this was the coldest chain
      if (id % chains_.numTemps() != 0)
      {
        // Lock the chain that is below (colder) than this.
        locked_[id - 1] = true;
      }
      else
      {
        // This is the coldest chain and there is no one to swap with
        propose(id);
      }
    }

  
  }
}
