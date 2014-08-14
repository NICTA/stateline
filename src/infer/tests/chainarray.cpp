//!
//! Contains tests for the chain array.
//!
//! \file infer/testchainarray.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "gtest/gtest.h"
#include "boost/filesystem.hpp"

#include "db/db.hpp"
#include "infer/chainarray.hpp"

using namespace stateline;
using namespace stateline::mcmc;

class ChainArrayTest : public testing::Test
{
  public:
    uint nChains = 3;
    uint nStacks = 2;
    Eigen::VectorXd sigma;
    double beta;
    std::vector<Eigen::VectorXd> sigmas;
    std::vector<double> betas;
    uint cacheLength = 2;
    std::string path = "./AUTOGENtestChainArray";
    DBSettings settings;

    ChainArrayTest()
    {
      settings.directory = path;
      settings.cacheSizeMB = 1.0;
      boost::filesystem::remove_all(path);

      sigma = Eigen::VectorXd::Ones(1);
      beta = 1.0;

      for (uint i = 0; i < nChains * nStacks; i++)
      {
        sigmas.push_back(sigma);
        betas.push_back(beta);
      }
    }

    ~ChainArrayTest()
    {
      boost::filesystem::remove_all(path);
    }
};

TEST_F(ChainArrayTest, chainsStartAtLengthZero)
{
  ChainArray chains(nStacks, nChains, sigmas, betas, settings, cacheLength);
  for (uint i = 0; i < chains.numTotalChains(); i++)
  {
    uint length = chains.length(i);
    EXPECT_EQ(0U, length);
  }
}

TEST_F(ChainArrayTest, canInitialiseChain)
{
  ChainArray chains(nStacks, nChains, sigmas, betas, settings, cacheLength);

  // Append a test sample to the chain
  Eigen::VectorXd m(5);
  m << 1.0, 2.0, 3.0, 4.0, 5.0;
  chains.initialise(0, m, 666.0);

  ASSERT_EQ(1U, chains.length(0));
  EXPECT_TRUE(chains.sigma(0).isApprox(sigma));
  EXPECT_DOUBLE_EQ(beta, chains.beta(0));
  
  EXPECT_DOUBLE_EQ(666.0, chains.lastState(0).energy);
  EXPECT_TRUE(chains.lastState(0).sample.isApprox(m));
  EXPECT_TRUE(chains.lastState(0).sigma.isApprox(sigma));
  EXPECT_DOUBLE_EQ(beta, chains.lastState(0).beta);
  EXPECT_EQ(true, chains.lastState(0).accepted);
  EXPECT_EQ(SwapType::NoAttempt, chains.lastState(0).swapType);
}

TEST_F(ChainArrayTest, canSwapChains)
{
  ChainArray chains(nStacks, nChains, sigmas, betas, settings, cacheLength);

  // Create test samples
  Eigen::VectorXd m1(5);
  m1 << 1.0, 2.0, 3.0, 4.0, 5.0;

  Eigen::VectorXd m2(5);
  m2 << -1.0, -2.0, -3.0, -4.0, -5.0;

  // Give the two chains different sigma and beta
  Eigen::VectorXd sigma1 = Eigen::VectorXd::Ones(1) * 0.1;
  Eigen::VectorXd sigma2 = Eigen::VectorXd::Ones(1) * 0.2;

  chains.setSigma(0, sigma1);
  chains.setSigma(1, sigma2);

  chains.setBeta(0, 0.1);
  chains.setBeta(1, 0.11);

  // Append to two separate chains and then swap them
  chains.initialise(0, m1, 666.0);
  chains.initialise(1, m2, 667.0);
  ASSERT_EQ(SwapType::Accept, chains.swap(0, 1));

  ASSERT_EQ(1U, chains.length(0));
  ASSERT_EQ(1U, chains.length(1));
  
  // Chain 0 should be m2
  EXPECT_DOUBLE_EQ(667.0, chains.lastState(0).energy);
  EXPECT_TRUE(chains.lastState(0).sample.isApprox(m2));
  EXPECT_TRUE(chains.lastState(0).sigma.isApprox(sigma2));
  EXPECT_DOUBLE_EQ(0.11, chains.lastState(0).beta);
  EXPECT_EQ(true, chains.lastState(0).accepted);
  EXPECT_EQ(SwapType::Accept, chains.lastState(0).swapType);
  
  // Chain 1 should be m1
  EXPECT_DOUBLE_EQ(666.0, chains.lastState(1).energy);
  EXPECT_TRUE(chains.lastState(1).sample.isApprox(m1));
  EXPECT_TRUE(chains.lastState(1).sigma.isApprox(sigma1));
  EXPECT_DOUBLE_EQ(0.1, chains.lastState(1).beta);
  EXPECT_EQ(true, chains.lastState(1).accepted);
  EXPECT_EQ(SwapType::Accept, chains.lastState(1).swapType);
}

TEST_F(ChainArrayTest, canRecoverChain)
{
  // Create test samples
  Eigen::VectorXd m1(5);
  m1 << 1.0, 2.0, 3.0, 4.0, 5.0;

  Eigen::VectorXd m2(5);
  m2 << -1.0, -2.0, -3.0, -4.0, -5.0;

  // Needs a separate scope to call the destructor on chain array because
  // we can't simultaneously read from the DB.
  {
    ChainArray chains(nStacks, nChains, sigmas, betas, settings, 2);
    chains.initialise(0, m1, 666.0);
    chains.append(0, m2, 333.0);
    chains.flushToDisk(0);
  }

  // Now recover it
  ChainArray recovered(settings, 2);

  // Note that the newest state (m2) is never saved to disk, so we only check whether
  // the first state (m1) is present.
  ASSERT_EQ(1U, recovered.length(0));
  EXPECT_DOUBLE_EQ(666.0, recovered.lastState(0).energy);
  EXPECT_TRUE(recovered.lastState(0).sample.isApprox(m1));
  EXPECT_TRUE(recovered.lastState(0).sigma.isApprox(sigma));
  EXPECT_DOUBLE_EQ(1.0, recovered.beta(0));
  EXPECT_EQ(true, recovered.lastState(0).accepted);
  EXPECT_EQ(SwapType::NoAttempt, recovered.lastState(0).swapType);
}
