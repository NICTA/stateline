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
    uint totalChains = nChains * nStacks;
    double tempFactor = 2.0;
    double initialSigma = 1.0;
    double sigmaFactor = 2.0;
    uint cacheLength = 2;
    std::string path = "./AUTOGENtestChainArray";
    DBSettings settings;

    ChainArrayTest()
    {
      settings.directory = path;
      settings.recover = false;
      settings.cacheSizeMB = 1.0;
      boost::filesystem::remove_all(path);
    }

    ~ChainArrayTest()
    {
      boost::filesystem::remove_all(path);
    }
};

TEST_F(ChainArrayTest, chainsStartAtLengthZero)
{
  ChainArray chains(nStacks, nChains, tempFactor, initialSigma, sigmaFactor, settings, cacheLength);
  for (uint i = 0; i < totalChains; i++)
  {
    uint length = chains.length(i);
    EXPECT_EQ(0U, length);
  }
}

TEST_F(ChainArrayTest, canInitialiseChain)
{
  ChainArray chains(nStacks, nChains, tempFactor, initialSigma, sigmaFactor, settings, 2);

  // Append a test sample to the chain
  Eigen::VectorXd m(5);
  m << 1.0, 2.0, 3.0, 4.0, 5.0;
  chains.initialise(0, State { m, 666.0, 1.0, 1.0, true, SwapType::NoAttempt });

  ASSERT_EQ(1U, chains.length(0));
  EXPECT_DOUBLE_EQ(666.0, chains.lastState(0).energy);
  EXPECT_TRUE(m.isApprox(chains.lastState(0).sample));
  EXPECT_DOUBLE_EQ(1.0, chains.beta(0));
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
    ChainArray chains(nStacks, nChains, tempFactor, initialSigma, sigmaFactor, settings, 2);
    chains.initialise(0, State { m1, 666.0, 1.0, 1.0, true, SwapType::NoAttempt });
    chains.append(0, State { m2, -666.0, 1.0, -1.0, false, SwapType::Accept });
    chains.flushToDisk(0);
  }

  // Now recover it
  DBSettings recoverSettings = settings;
  recoverSettings.recover = true;

  ChainArray recovered(nStacks, nChains, tempFactor, initialSigma, sigmaFactor, recoverSettings, 2);

  // Note that the newest state (m2) is never saved to disk, so we only check whether
  // the first state (m1) is present.
  ASSERT_EQ(1U, recovered.length(0));
  EXPECT_DOUBLE_EQ(666.0, recovered.lastState(0).energy);
  EXPECT_TRUE(m1.isApprox(recovered.lastState(0).sample));
  EXPECT_DOUBLE_EQ(1.0, recovered.beta(0));
}
