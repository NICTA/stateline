//!
//! Contains the interface of an MCMC sampler.
//!
//! \file infer/sampler.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "../comms/requester.hpp"
#include "../infer/datatypes.hpp"
#include "../infer/chainarray.hpp"

#include <json.hpp>

namespace stateline
{
  namespace mcmc
  {

    Eigen::VectorXd bouncyBounds(const Eigen::VectorXd& val,
        const Eigen::VectorXd& min, const Eigen::VectorXd& max);

    //! Settings for the defining a hard boundary on the samples produced
    //! by the proposal function.
    struct ProposalBounds
    {
      Eigen::VectorXd min;
      Eigen::VectorXd max;

      static ProposalBounds fromJSON(const nlohmann::json& j)
      {
        ProposalBounds b;
        uint nDims = j["dimensionality"];
        uint nMin = j["boundaries"]["min"].size();
        uint nMax = j["boundaries"]["max"].size();

        if ((nMin != 0) || (nMax != 0))
        {
          if ((nMin != nDims) || (nMax != nDims))
          {
            LOG(ERROR) << "Proposal bounds dimension mismatch: ndim="
                       << nDims <<", nMin=" << nMin << ", nMax=" << nMax;
          }
          else
          {
            b.min.resize(nDims);
            b.max.resize(nDims);
            for (uint i=0; i < nDims; ++i)
            {
              b.min[i] = j["boundaries"]["min"][i];
              b.max[i] = j["boundaries"]["max"][i];
            }
          }
        }
        return b;
      }
    };

    using ProposalFunction = std::function<Eigen::VectorXd(uint id, const Eigen::VectorXd &sample, double sigma)>;

    class GaussianCovProposal
    {
      public:
        GaussianCovProposal(uint nStacks, uint nChains, uint nDims, const ProposalBounds& bounds);

        Eigen::VectorXd propose(uint id, const Eigen::VectorXd &sample, double sigma);
        Eigen::VectorXd boundedPropose(uint id, const Eigen::VectorXd &sample, double sigma);

        Eigen::VectorXd operator()(uint id, const Eigen::VectorXd &sample, double sigma);

        void update(uint id, const Eigen::MatrixXd &cov);

      private:
        std::mt19937 gen_;
        std::normal_distribution<> rand_; // Standard normal
        std::vector<Eigen::MatrixXd> sigL_;

        ProposalBounds bounds_;
        ProposalFunction proposeFn_;
    };

    class Sampler
    {
      public:
        Sampler(comms::Requester& requester, 
                std::vector<uint> jobTypes,
                ChainArray& chainArray,
                const ProposalFunction& propFn,
                uint swapInterval);

        ~Sampler();
      
        std::pair<uint, State> step(const std::vector<double>& sigmas, const std::vector<double>& betas);

        void flush();

      private:

        void propose(uint id);

        void unlock(uint id);

        comms::Requester& requester_;

        std::vector<uint> jobTypes_;

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
