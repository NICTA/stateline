//!
//! \file stats/testtruncnormal.cpp
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "stats/truncnormal.hpp"

#include <gtest/gtest.h>
#include <random>

using namespace stateline;

TEST(TruncNormalDistribution, canNotGetOutOfBoundsSample)
{
  Eigen::MatrixXd x(4, 5);
  x << 1, 2, 3, 4, 5,
       3, 0, 3, 0, -3,
       0, -10, 1, 3, 4,
       0, 0, 0, 0, 0;

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

  stats::TruncNormal d(mean, cov, min, max);
  
  std::mt19937 rng(19937);
  for (int i = 0; i < 100; i++)
  {
    EXPECT_TRUE(stats::insupport(d, stats::sample(d, rng)));
  }
}
