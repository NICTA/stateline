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

    GaussianProposal::GaussianProposal(uint nStacks, uint nChains, uint
            nDims, const ProposalBounds& bounds, uint init_length)
      : gen_(std::random_device()()), 
      bounds_(bounds), 
      proposalShape_(nStacks, nChains, nDims, bounds, init_length)
    {

      if ((bounds_.min.rows() == nDims) && (bounds_.max.rows() == nDims))
      {
        LOG(INFO) << "Using a bounded Gaussian proposal function";
        proposeFn_ = std::bind(&GaussianProposal::boundedPropose, this,
                ph::_1, ph::_2, ph::_3);
      }
      else
      {
        LOG(INFO) << "Using a Gaussian proposal function";
        proposeFn_ = std::bind(&GaussianProposal::propose, this, ph::_1,
                ph::_2, ph::_3);
      }
    }

    Eigen::VectorXd GaussianProposal::propose(uint id, const Eigen::VectorXd &sample, double sigma)
    {
      uint n = sample.size();

      Eigen::VectorXd randn(n);
      for (uint i = 0; i < n; i++)
        randn(i) = rand_(gen_);

      return sample + proposalShape_.Ns()[id] * randn * sigma;
    }

    Eigen::VectorXd GaussianProposal::boundedPropose(uint id, const Eigen::VectorXd& sample, double sigma)
    {
      return bouncyBounds(propose(id, sample, sigma), bounds_.min, bounds_.max);
    }

    Eigen::VectorXd GaussianProposal::operator()(uint id, const Eigen::VectorXd &sample, double sigma)
    {
      return proposeFn_(id, sample, sigma);
    }

    void GaussianProposal::update(uint id, const Eigen::VectorXd &stepv)
    {
      proposalShape_.update(id, stepv);
    }
    
    //ProposalFunction& proposal,
    Sampler::Sampler(comms::Requester& requester, 
                     ChainArray& chainArray,
                     mcmc::GaussianProposal& proposal, 
                     RegressionAdapter& sigmaAdapter,
                     RegressionAdapter& betaAdapter,
                     uint swapInterval)
      : requester_(requester),
        chains_(chainArray),
        proposal_(proposal),
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
      State previous_state = chains_.lastState(id);
      chains_.append(id, propStates_[id], energy);  // TODO: return something?
      State state = chains_.lastState(id); 
      haveFlushed_ = false;

      // Adapt sigma (the scale of the proposal):
      double logtemper = -log(state.beta); 
      sigmaAdapter_.update(id, log(state.sigma), logtemper, state.accepted);
      chains_.setSigma(id, sigmaAdapter_.computeSigma(id, logtemper));

      // Adapt the proposal shape:
      if (state.accepted)
          proposal_.update(id, state.sample - previous_state.sample);

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

        if (chains_.isColdestInStack(id))
            betaAdapter_.computeBetaStack(id);

        // We just finished an interval - set last interval's optimum
        // temperature to the guy we just unlocked...
        chains_.setBeta(id+1, betaAdapter_.values()[id+1]);
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
      // todo(Al) - should we be getting this from the chains directly?
      double sigma = sigmaAdapter_.values()[id];
      const auto prop = proposal_(id, chains_.lastState(id).sample, sigma);
      std::vector<double> data(prop.data(), prop.data() + prop.size());
      requester_.submit(id, data);
      propStates_[id] = prop;
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
