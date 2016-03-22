//!
//! Contains tests for the chain array.
//!
//! \file infer/tests/diagnostics.hpp
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "gtest/gtest.h"

#include "infer/diagnostics.hpp"
     
using namespace stateline;
using namespace stateline::mcmc;

TEST(DiagnosticsTest, EPSRConstantChains)
{
  // Create 5 chains which have 10 samples filled with 0
  Eigen::MatrixXd x = Eigen::MatrixXd::Zero(10, 5);

  EPSRDiagnostic epsr(5, 1, 1);
  for (int i = 0; i < 10; i++)
  {
    for (int j = 0; j < 5; j++)
    {
      State state;
      state.sample = Eigen::VectorXd::Ones(1) * x(i, j);
      epsr.update(j, state);
    }
  }

  EXPECT_EQ(0.0, epsr.rHat()(0));
}

TEST(DiagnosticsTest, EPSRDuplicateLinSpacedChains)
{
  // Create 5 chains which have the same samples
  Eigen::VectorXd chain = Eigen::VectorXd::LinSpaced(10, 0, 1);

  EPSRDiagnostic epsr(5, 1, 1);
  for (int i = 0; i < 5; i++)
  {
    for (int j = 0; j < chain.rows(); j++)
    {
      State state;
      state.sample = Eigen::VectorXd::Ones(1) * chain(j);
      epsr.update(i, state);
    }
  }

  EXPECT_NEAR(0.948683298050513, epsr.rHat()(0), 1e-10);
}

TEST(DiagnosticsTest, EPSRRandomChains)
{
  // Create 2 chains which have random samples
  Eigen::MatrixXd x(5, 2);
  x <<
    0.1415084, 0.8388452,
    0.3565489, 0.8506014,
    0.1773983, 0.2258481,
    0.6900898, 0.6106332,
    0.0096742, 0.6742046;

  EPSRDiagnostic epsr(2, 1, 1);
  for (int i = 0; i < x.rows(); i++)
  {
    State state1;
    state1.sample = Eigen::VectorXd::Ones(1) * x(i, 0);
    epsr.update(0, state1);
    State state2;
    state2.sample = Eigen::VectorXd::Ones(1) * x(i, 1); 
    epsr.update(1, state2);
  }

  EXPECT_NEAR(1.340739719234503, epsr.rHat()(0), 1e-10);
}
