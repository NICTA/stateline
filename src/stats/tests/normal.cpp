//!
//! \file stats/testnormal.cpp
//! \author Alistair Reid
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "stats/normal.hpp"

#include <gtest/gtest.h>
#include <limits>

using namespace stateline;

TEST(NormalDistribution, unnormalisedPdfHasSameConstantFactor)
{
  Eigen::MatrixXd x(4, 5);
  x << 1, 2, 3, 4, 5,
       3, 0, 3, 0, -3,
       0, -10, 1, 3, 4,
       0, 0, 0, 0, 0;

  Eigen::VectorXd mean(5);
  mean << 3, 4, 3, 2, 3;

  Eigen::MatrixXd cov(5, 5);
  cov << 15, 9, 11, 7, 14,
         9, 10, 5, 8, 12,
         11, 5, 11, 3, 8,
         7, 8, 3, 9, 9,
         14, 12, 8, 9, 19;

  stats::Normal d(mean, cov);
  
  double pdfAtMean = stats::logpdf(d, stats::mean(d));
  double lognorm = -7.99589004769 - pdfAtMean;

  EXPECT_NEAR(-20.7958900477, stats::logpdf(d, x.row(0)) + lognorm, 1e-6);
  EXPECT_NEAR(-12.3958900477, stats::logpdf(d, x.row(1)) + lognorm, 1e-6);
  EXPECT_NEAR(-106.595890048, stats::logpdf(d, x.row(2)) + lognorm, 1e-6);
  EXPECT_NEAR(-9.69589004769, stats::logpdf(d, x.row(3)) + lognorm, 1e-6);
}
