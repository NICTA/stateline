//!
//! \file stats/testnormal.cpp
//! \author Alistair Reid
//! \author Darren Shen
//! \date 2014-05-27
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "normal.hpp"

#include <gtest/gtest.h>
#include <limits>

using namespace stateline;

TEST(NormalDistribution, hasCorrectPDF)
{
  Eigen::VectorXd x(5);
  x << 1, 2, 3, 4, 5;

  Eigen::VectorXd mean(5);
  mean << 3, 4, 3, 2, 3;

  Eigen::VectorXd min = Eigen::VectorXd::Zero(5);
  Eigen::VectorXd max = Eigen::VectorXd::Ones(5) * 5;

  Eigen::MatrixXd cov(5, 5);
  cov << 15, 9, 11, 7, 14,
         9, 10, 5, 8, 12,
         11, 5, 11, 3, 8,
         7, 8, 3, 9, 9,
         14, 12, 8, 9, 19;

  stats::Normal d(mean, cov);
  EXPECT_NEAR(-20.7959, stats::logpdf(d, x), 0.001);

 // maxs = Eigen::VectorXd::Ones(5) * 4.9;
 // answer = distrib::logPDF(X, gauss, mins, maxs);
 // EXPECT_TRUE(is_neg_infinity(answer));
 // maxs = Eigen::VectorXd::Ones(5) * 6;
 // mins = Eigen::VectorXd::Ones(5) * 1.1;
 // answer = distrib::logPDF(X, gauss, mins, maxs);
 // EXPECT_TRUE(is_neg_infinity(answer));
 // mins = Eigen::VectorXd::Ones(5) * 1;
 // answer = distrib::logPDF(X, gauss, mins, maxs);
 // EXPECT_NEAR(answer, -20.7959, 0.1);
}
