//!
//! Contains tests for the chain array.
//!
//! \file infer/tests/diagnostics.hpp
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "gtest/gtest.h"

#include "infer/diagnostics.hpp"
     
using namespace stateline;
using namespace stateline::mcmc;

TEST(DiagnosticsTest, EpsrConstantChains)
{
  // Create 5 chains which have 10 samples filled with 0
  Eigen::MatrixXd x = Eigen::MatrixXd::Zero(10, 5);

  EpsrConvergenceCriteria epsr(5, 1);
  for (int i = 0; i < 10; i++)
  {
    for (int j = 0; j < 5; j++)
    {
      epsr.update(j, Eigen::VectorXd::Ones(1) * x(i, j));
    }
  }

  EXPECT_EQ(0.0, epsr.rHat()(0));
}

TEST(DiagnosticsTest, EpsrDuplicateLinSpacedChains)
{
  // Create 5 chains which have the same samples
  Eigen::VectorXd chain = Eigen::VectorXd::LinSpaced(10, 0, 1);

  EpsrConvergenceCriteria epsr(5, 1);
  for (int i = 0; i < 5; i++)
  {
    for (int j = 0; j < chain.rows(); j++)
    {
      epsr.update(i, Eigen::VectorXd::Ones(1) * chain(j));
    }
  }

  EXPECT_NEAR(0.948683298050513, epsr.rHat()(0), 1e-10);
}

TEST(DiagnosticsTest, EpsrRandomChains)
{
  // Create 2 chains which have random samples
  Eigen::MatrixXd x(5, 2);
  x <<
    0.1415084, 0.8388452,
    0.3565489, 0.8506014,
    0.1773983, 0.2258481,
    0.6900898, 0.6106332,
    0.0096742, 0.6742046;

  EpsrConvergenceCriteria epsr(2, 1);
  for (int i = 0; i < x.rows(); i++)
  {
    epsr.update(0, Eigen::VectorXd::Ones(1) * x(i, 0));
    epsr.update(1, Eigen::VectorXd::Ones(1) * x(i, 1));
  }

  EXPECT_NEAR(1.340739719234503, epsr.rHat()(0), 1e-10);
}
