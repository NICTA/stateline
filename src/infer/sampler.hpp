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
#include "../infer/adaptive.hpp"
#include "../infer/chainarray.hpp"
#include "../app/json.hpp"

#include <random>

namespace stateline
{
  namespace mcmc
  {

    Eigen::VectorXd bouncyBounds(const Eigen::VectorXd& val,
        const Eigen::VectorXd& min, const Eigen::VectorXd& max);

    //! Settings for the defining a hard boundary on the samples produced
    //! by the proposal function.

    // TODO: this should not be here.
    static ProposalBounds ProposalBoundsFromJSON(const nlohmann::json& j)
    {
        std::vector<double> vmin, vmax;
        readFields(j, "min", vmin);
        readFields(j, "max", vmax);

        if (vmin.size() != vmax.size())
        {
            LOG(FATAL) << "Proposal bounds dimension mismatch: nMin=" 
                << vmin.size() << ", nMax=" << vmax.size();
        }

        ProposalBounds b;
        b.min.resize(vmin.size());
        b.max.resize(vmax.size());
        for (std::size_t i = 0; i < vmin.size(); i++)
        {
          b.min(i) = vmin[i];
          b.max(i) = vmax[i];
        }
        return b;
    }

    using ProposalFunction = std::function<Eigen::VectorXd(uint id, const Eigen::VectorXd &sample, double sigma)>;

    class GaussianProposal
    {
      public:
        GaussianProposal(uint nStacks, uint nChains, uint nDims, 
                const ProposalBounds& bounds, uint init_length);

        Eigen::VectorXd propose(uint id, const Eigen::VectorXd &sample, double sigma);
        Eigen::VectorXd boundedPropose(uint id, const Eigen::VectorXd &sample, double sigma);

        Eigen::VectorXd operator()(uint id, const Eigen::VectorXd &sample, double sigma);

        void update(uint id, const Eigen::VectorXd &sample);

      private:
        std::mt19937 gen_;
        std::normal_distribution<> rand_; // Standard normal generator

        ProposalBounds bounds_;
        ProposalFunction proposeFn_;

        mcmc::ProposalShaper proposalShape_;
    };

    class Sampler
    {
      public:
        // look into ProposalFunction& proposal
        Sampler(comms::Requester& requester, 
                ChainArray& chainArray,
                mcmc::GaussianProposal& proposal, 
                RegressionAdapter& sigmaAdapter,
                RegressionAdapter& betaAdapter,
                uint swapInterval);

        ~Sampler();
      
        std::pair<uint, State> step();

        void flush();

      private:

        void propose(uint id);

        void unlock(uint id);

        comms::Requester& requester_;

        // The MCMC chain wrapper
        ChainArray& chains_;
        
        // Not a reference because possibly std function?
        ///ProposalFunction proposal_;
        // Actually, now its always a Gaussian Covariance which simplifies
        // adaption
        mcmc::GaussianProposal& proposal_; 

        RegressionAdapter& sigmaAdapter_;
        RegressionAdapter& betaAdapter_;

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
