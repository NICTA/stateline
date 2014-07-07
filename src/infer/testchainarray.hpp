//!
//! Contains tests for the chain array.
//!
//! \file infer/testchainarray.hpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <glog/logging.h>
#include "gtest/gtest.h"
#include "boost/filesystem.hpp"

#include "db/db.hpp"
#include "infer/chainarray.hpp"


     
namespace stateline
{
  namespace mcmc
  {
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
      using namespace internal;

      db::Database db(settings);
      ChainArray chains(nStacks, nChains, tempFactor, initialSigma, sigmaFactor, db, cacheLength, false);
      for (uint i = 0; i < totalChains; i++)
      {
        uint length = chains.length(i);
        EXPECT_EQ(length, 0U);
      }
    }

    TEST_F(ChainArrayTest, canSerialiseStringsToDb)
    {
      using namespace internal;

      db::Database db(settings);
      std::string s1 = toDbString(0,0, DbEntryType::STATE);
      std::string s2 = toDbString(0,1, DbEntryType::STATE);
      std::string s3 = toDbString(1,0, DbEntryType::STATE);
      std::string s4 = toDbString(1,1, DbEntryType::STATE);
      db.put(s1, "s1");
      db.put(s2, "s2");
      db.put(s3, "s3");
      db.put(s4, "s4");
      std::string r1 = db.get(s1);
      std::string r2 = db.get(s2);
      std::string r3 = db.get(s3);
      std::string r4 = db.get(s4);
      EXPECT_EQ(r1,"s1");
      EXPECT_EQ(r2,"s2");
      EXPECT_EQ(r3,"s3");
      EXPECT_EQ(r4,"s4");
    }
    
    TEST_F(ChainArrayTest, canInitialiseChain)
    {
      using namespace internal;

      db::Database db(settings);
      ChainArray chains(nStacks, nChains, tempFactor, initialSigma, sigmaFactor, db, 2, false);

      // Append a test sample to the chain
      Eigen::VectorXd m(5);
      m << 1.0, 2.0, 3.0, 4.0, 5.0;
      chains.initialise(0, State { m, 666.0, 1.0, true, SwapType::NoAttempt });

      ASSERT_EQ(1U, chains.length(0));
      EXPECT_DOUBLE_EQ(666.0, chains.lastState(0).energy);
      EXPECT_TRUE(m.isApprox(chains.lastState(0).sample));
      EXPECT_DOUBLE_EQ(1.0, chains.beta(0));
    }
    
    //TEST_F(ChainArrayTest, canAppendToDifferentChains)
    //{
    //  db::Database db(settings);
    //  ChainArray chains(nStacks, nChains, tempFactor, initialSigma, sigmaFactor, db, 2, false);

    //  Eigen::VectorXd s1(5);
    //  s1 << 1.0, 2.0, 3.0, 4.0, 5.0;
    //  Eigen::VectorXd s2(5);
    //  s2 << 1.0, 3.0, 2.0, 4.0, 5.0;
    //  Eigen::VectorXd s3(5);
    //  s3 << 1.0, 2.0, 3.0, 4.0, 6.0;
    //  Eigen::VectorXd s4(5);
    //  s4 << 1.0, 3.0, 2.0, 4.0, 9.0;

    //  chains.initialise(0, { s1, 666.0, 1.0, true, SwapType::NoAttempt });
    //  chains.initialise(1, { s2, 888.0, 2.0, false, SwapType::NoAttempt });
    //  chains.append(0, { s3, 777.0, 3.0, true, SwapType::NoAttempt });
    //  chains.append(1, { s4, 999.0, 4.0, false, SwapType::NoAttempt });

    //  ASSERT_EQ(2U, chains.length(0));
    //  ASSERT_EQ(2U, chains.length(1));

    //  EXPECT_TRUE(s1.isApprox(chains.state(0, 0).sample));
    //  EXPECT_EQ(666.0, chains.state(0, 0).energy);
    //  EXPECT_EQ(1.0, chains.state(0, 0).beta);
    //  EXPECT_TRUE(chains.state(0, 0).accepted);
    //  EXPECT_EQ(SwapType::NoAttempt, chains.state(0, 0).swapType);
    //
    //  EXPECT_TRUE(s2.isApprox(chains.state(1, 0).sample));
    //  EXPECT_EQ(888.0, chains.state(1, 0).energy);
    //  EXPECT_EQ(2.0, chains.state(1, 0).beta);
    //  EXPECT_FALSE(chains.state(1, 0).accepted);
    //  EXPECT_EQ(SwapType::NoAttempt, chains.state(1, 0).swapType);

    //  EXPECT_TRUE(s3.isApprox(chains.state(0, 1).sample));
    //  EXPECT_EQ(777.0, chains.state(0, 1).energy);
    //  EXPECT_EQ(3.0, chains.state(0, 1).beta);
    //  EXPECT_TRUE(chains.state(0, 1).accepted);
    //  EXPECT_EQ(SwapType::NoAttempt, chains.state(0, 1).swapType);

    //  EXPECT_TRUE(s4.isApprox(chains.state(1, 1).sample));
    //  EXPECT_EQ(999.0, chains.state(1, 1).energy);
    //  EXPECT_EQ(4.0, chains.state(1, 1).beta);
    //  EXPECT_FALSE(chains.state(1, 1).accepted);
    //  EXPECT_EQ(SwapType::NoAttempt, chains.state(1, 1).swapType);
    //}
    
    // TEST_F(ChainArrayTest, SwapStatesAccept)
    // {
    //   db::Database db(settings);
    //   ChainArray chains(nStacks, nChains, 5, tempFactor, sigma, db, false);
    //   Eigen::VectorXd m(5);
    //   m << 1.0, 2.0, 3.0,4.0,5.0;
    //   Eigen::VectorXd n(5);
    //   n << 1.0, 3.0, 2.0,4.0,5.0;
    //   Eigen::VectorXd o(5);
    //   o << 1.0, 2.0, 3.0,4.0,6.0;
    //   Eigen::VectorXd p(5);
    //   p << 1.0, 3.0, 2.0,4.0,9.0;

    //   chains.append(0, m, 666.0, 1.0, true);
    //   chains.append(1, n, 888.0, 2.0, false);
    //   chains.append(0, o, 777.0, 3.0, true);
    //   chains.append(1, p, 999.0, 4.0, false);

    //   chains.swapLast(0,1);

    //   Eigen::VectorXd r1 = chains.state(0,0); 
    //   double e1 = chains.energy(0,0);
    //   double b1 = chains.beta(0,0);
    //   bool a1 = chains.acceptance(0,0);
    //   SwapType s1 = chains.swapType(0,0);
    //   Eigen::VectorXd r2 = chains.state(1,0); 
    //   double e2 = chains.energy(1,0);
    //   double b2 = chains.beta(1,0);
    //   bool a2 = chains.acceptance(1,0);
    //   SwapType s2 = chains.swapType(1,0);
    //   Eigen::VectorXd r3 = chains.state(0,1); 
    //   double e3 = chains.energy(0,1);
    //   double b3 = chains.beta(0,1);
    //   bool a3 = chains.acceptance(0,1);
    //   SwapType s3 = chains.swapType(0,1);
    //   Eigen::VectorXd r4 = chains.state(1,1); 
    //   double e4 = chains.energy(1,1);
    //   double b4 = chains.beta(1,1);
    //   bool a4 = chains.acceptance(1,1);
    //   SwapType s4 = chains.swapType(1,1);
    //   EXPECT_EQ(r1,m);
    //   EXPECT_EQ(r2,n);
    //   EXPECT_EQ(r4,o);
    //   EXPECT_EQ(r3,p);
    //   EXPECT_EQ(e1, 666.0);
    //   EXPECT_EQ(e2, 888.0);
    //   EXPECT_EQ(e4, 777.0);
    //   EXPECT_EQ(e3, 999.0);
    //   EXPECT_EQ(b1, 1.0);
    //   EXPECT_EQ(b2, 2.0);
    //   EXPECT_EQ(b3, 3.0);
    //   EXPECT_EQ(b4, 4.0); // note not swapped
    //   EXPECT_EQ(a1, true);
    //   EXPECT_EQ(a2, false);
    //   EXPECT_EQ(a4, true);
    //   EXPECT_EQ(a3, false);
    //   EXPECT_EQ(s1,SwapType::NoAttempt);
    //   EXPECT_EQ(s2,SwapType::NoAttempt);
    //   EXPECT_EQ(s3,SwapType::Accept);
    //   EXPECT_EQ(s4,SwapType::Accept);
    // }
    
    // TEST_F(ChainArrayTest, SwapStatesReject)
    // {
    //   db::Database db(settings);
    //   ChainArray chains(nStacks, nChains, 5, tempFactor, sigma, db, false);
    //   Eigen::VectorXd m(5);
    //   m << 1.0, 2.0, 3.0,4.0,5.0;
    //   Eigen::VectorXd n(5);
    //   n << 1.0, 3.0, 2.0,4.0,5.0;
    //   Eigen::VectorXd o(5);
    //   o << 1.0, 2.0, 3.0,4.0,6.0;
    //   Eigen::VectorXd p(5);
    //   p << 1.0, 3.0, 2.0,4.0,9.0;

    //   chains.append(0, m, 666.0, 1.0, true);
    //   chains.append(1, n, 888.0, 2.0, false);
    //   chains.append(0, o, 777.0, 3.0, true);
    //   chains.append(1, p, 999.0, 4.0, false);

    //   chains.rejectedSwap(0,1);

    //   Eigen::VectorXd r1 = chains.state(0,0); 
    //   double e1 = chains.energy(0,0);
    //   double b1 = chains.beta(0,0);
    //   bool a1 = chains.acceptance(0,0);
    //   SwapType s1 = chains.swapType(0,0);

    //   Eigen::VectorXd r2 = chains.state(1,0); 
    //   double e2 = chains.energy(1,0);
    //   double b2 = chains.beta(1,0);
    //   bool a2 = chains.acceptance(1,0);
    //   SwapType s2 = chains.swapType(1,0);
    //   Eigen::VectorXd r3 = chains.state(0,1); 
    //   double e3 = chains.energy(0,1);
    //   double b3 = chains.beta(0,1);
    //   bool a3 = chains.acceptance(0,1);
    //   SwapType s3 = chains.swapType(0,1);
    //   Eigen::VectorXd r4 = chains.state(1,1); 
    //   double e4 = chains.energy(1,1);
    //   double b4 = chains.beta(1,1);
    //   bool a4 = chains.acceptance(1,1);
    //   SwapType s4 = chains.swapType(1,1);
    //   EXPECT_EQ(r1,m);
    //   EXPECT_EQ(r2,n);
    //   EXPECT_EQ(r3,o);
    //   EXPECT_EQ(r4,p);
    //   EXPECT_EQ(e1, 666.0);
    //   EXPECT_EQ(e2, 888.0);
    //   EXPECT_EQ(e3, 777.0);
    //   EXPECT_EQ(e4, 999.0);
    //   EXPECT_EQ(b1, 1.0);
    //   EXPECT_EQ(b2, 2.0);
    //   EXPECT_EQ(b3, 3.0);
    //   EXPECT_EQ(b4, 4.0);
    //   EXPECT_EQ(a1, true);
    //   EXPECT_EQ(a2, false);
    //   EXPECT_EQ(a3, true);
    //   EXPECT_EQ(a4, false);
    //   EXPECT_EQ(s1,SwapType::NoAttempt);
    //   EXPECT_EQ(s2,SwapType::NoAttempt);
    //   EXPECT_EQ(s3,SwapType::Reject);
    //   EXPECT_EQ(s4,SwapType::Reject);
    // }
    // TEST_F(ChainArrayTest, RetreiveLastState)
    // {
    //   db::Database db(settings);
    //   ChainArray chains(nStacks, nChains, 5, tempFactor, sigma, db, false);
    //   Eigen::VectorXd m(5);
    //   m << 1.0, 2.0, 3.0,4.0,5.0;
    //   chains.append(1, m, 666.0, 1.0, true);
    //   Eigen::VectorXd n(5);
    //   n << 1.0, 3.0, 2.0,4.0,5.0;
    //   chains.append(1, n, 777.0, 2.0, false);

    //   Eigen::VectorXd r = chains.lastState(1); 
    //   double e = chains.lastEnergy(1);
    //   double b = chains.lastBeta(1);
    //   bool accept = chains.lastAcceptance(1);
    //   SwapType swap = chains.lastSwapType(1);
    //   EXPECT_EQ(r,n);
    //   EXPECT_EQ(e,777.0);
    //   EXPECT_EQ(b,2.0);
    //   EXPECT_EQ(accept, false);
    //   EXPECT_EQ(swap, SwapType::NoAttempt);
    // }
    
  } // namespace db
} // namespace obsidian
