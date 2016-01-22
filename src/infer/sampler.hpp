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
#include "../app/jsonsettings.hpp"

#include <json.hpp>
#include <random>

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
        std::vector<double> vmin = readSettings<std::vector<double>>(j, "min");
        std::vector<double> vmax = readSettings<std::vector<double>>(j, "max");

        uint nMin = vmin.size();
        uint nMax = vmax.size();
        if (nMin != nMax)
        {
          LOG(FATAL) << "Proposal bounds dimension mismatch: nMin=" 
              << nMin << ", nMax=" << nMax;
        }
        else
        {
            uint nDims = nMax;
            b.min.resize(nDims);
            b.max.resize(nDims);
            for (uint i=0; i < nDims; ++i)
            {
              b.min[i] = vmin[i];
              b.max[i] = vmax[i];
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

        void update(uint id, const Eigen::VectorXd &sample);

      private:
        std::mt19937 gen_;
        std::normal_distribution<> rand_; // Standard normal
        std::vector<Eigen::MatrixXd> sigL_;

        ProposalBounds bounds_;
        ProposalFunction proposeFn_;

        mcmc::CovarianceEstimator covEstimator_;
    };

    class Sampler
    {
      public:
        // look into ProposalFunction& proposal
        Sampler(comms::Requester& requester, 
                std::vector<uint> jobTypes,
                ChainArray& chainArray,
                mcmc::GaussianCovProposal& proposal, 
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

        std::vector<uint> jobTypes_;

        // The MCMC chain wrapper
        ChainArray& chains_;
        
        // Not a reference because possibly std function?
        ///ProposalFunction proposal_;
        // Actually, now its always a Gaussian Covariance which simplifies
        // adaption
        mcmc::GaussianCovProposal& proposal_; 

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
