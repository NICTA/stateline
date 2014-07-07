//!
//! Test for c++ logNormPDF implementation and bounds.
//!
//! \file distrib/testgaussian.cpp
//! \author Alistair Reid
//! \author Nahid Akbar
//! \date 2014-05-27
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "multigaussian.hpp"
#include "datatype/datatypes.hpp"

#include <gtest/gtest.h>
#include <limits>

namespace obsidian
{

  class Test: public ::testing::Test
  {
  };

TEST_F(Test, testLogNormPDF)
{
  Eigen::VectorXd X (5);
  X << 1, 2, 3, 4, 5;
  Eigen::VectorXd mu (5);
  mu << 3, 4, 3, 2, 3;
  Eigen::VectorXd mins = Eigen::VectorXd::Zero(5);
  Eigen::VectorXd maxs = Eigen::VectorXd::Ones(5) * 5;

  Eigen::MatrixXd Sigma (5, 5);
  Sigma << 15, 9, 11, 7, 14,
  9, 10, 5, 8, 12,
  11, 5, 11, 3, 8,
  7, 8, 3, 9, 9,
  14, 12, 8, 9, 19;
  distrib::MultiGaussian gauss(mu, Sigma);
  double answer = distrib::logPDF(X, gauss, mins, maxs);
  EXPECT_NEAR(answer, -20.7959, 0.1);
  maxs = Eigen::VectorXd::Ones(5) * 4.9;
  answer = distrib::logPDF(X, gauss, mins, maxs);
  EXPECT_TRUE(is_neg_infinity(answer));
  maxs = Eigen::VectorXd::Ones(5) * 6;
  mins = Eigen::VectorXd::Ones(5) * 1.1;
  answer = distrib::logPDF(X, gauss, mins, maxs);
  EXPECT_TRUE(is_neg_infinity(answer));
  mins = Eigen::VectorXd::Ones(5) * 1;
  answer = distrib::logPDF(X, gauss, mins, maxs);
  EXPECT_NEAR(answer, -20.7959, 0.1);
}

//TEST_F(Test, testFiniteness)
//{
//  double d = -std::numeric_limits<double>::max();
//  EXPECT_TRUE(not std::isinf(d + d));
//}

}

int main(int argc, char **argv)
{
::testing::InitGoogleTest(&argc, argv);
return RUN_ALL_TESTS();
}

