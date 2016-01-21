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

      // Retrieve a result {id, likelihood factors}
      auto result = requester_.retrieve();
      uint id = result.first;
      double energy = 0.0;
      for (const auto& r : result.second)
        energy += r;
      numOutstandingJobs_--;

      // Advance the Markov chain (accept or reject logic inside append)
      chains_.append(id, propStates_[id], energy);  // TODO: return something?
      State state = chains_.lastState(id); 
      haveFlushed_ = false;

      // Adapt sigma:
      double temper = 1./state.beta;
      sigmaAdapter_.update(id, state.sigma(id), temper, state.accepted);
      proposal_.update(id, state.sample);
      chains_.setSigma(id, sigmaAdapter_.sigma(id, temper));

      // Apply swapping logic:
      if (locked_[id])
      {
        // The chain above is waiting -> attempt a swap
        // TODO(Al) - make swap return a bool?
        bool swapped = chains_.swap(id, id + 1) == SwapType::Accept;  

        // Propagate the swap to the rung below:
        unlock(id);  // Proposes for id+1 and locks id-1

        // Apply temperature update logic:
        betaAdapter_.betaUpdate(id, chains_.beta(id), chains_.beta(id+1), swapped);
        if chains_.isColdestInStack(id)
        {
            // Cache a new beta vector for this STACK 
            betaAdapter_.computeBetaStack(id);
        }
        else
        {
            // This is a good time to update the temperature because it is
            // right at the beginning of a new swap interval.
            // Note - this introduces a lag of one update interval
            chains_.setBeta(id, betaAdapter_.values()[id];
        }
      }
      else if (chains_.isHottestInStack(id)
              && chains_.length(id) % swapInterval_ == 0 
              && chains_.numTemps() > 1)
      {
        // Start a swap cascade from the hottest chain.
        locked_[id - 1] = true;
      }
      else
      {
        // No swap attempt - continue chain.
        propose(id);  
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
