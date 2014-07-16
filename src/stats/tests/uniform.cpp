//!
//! \file stats/testuniform.cpp
//! \author Darren Shen
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "stats/uniform.hpp"

#include <gtest/gtest.h>

using namespace stateline;

TEST(UniformDistribution, meanIsZeroForSymmetric)
{
  Eigen::Vector4d bound(1.0, 2.0, 3.0, 4.0);

  stats::Uniform d(-bound, bound);
  Eigen::VectorXd mean = stats::mean(d);

  ASSERT_EQ(4, mean.rows());
  EXPECT_DOUBLE_EQ(0.0, mean(0));
  EXPECT_DOUBLE_EQ(0.0, mean(1));
  EXPECT_DOUBLE_EQ(0.0, mean(2));
  EXPECT_DOUBLE_EQ(0.0, mean(3));
}

TEST(UniformDistribution, meanIsCorrectForAsymmetric)
{
  Eigen::Vector2d min(-3.0, 2.0);
  Eigen::Vector2d max(-2.0, 4.0);

  stats::Uniform d(min, max);
  Eigen::VectorXd mean = stats::mean(d);

  ASSERT_EQ(2, mean.rows());
  EXPECT_DOUBLE_EQ(-2.5, mean(0));
  EXPECT_DOUBLE_EQ(3.0, mean(1));
}

TEST(UniformDistribution, canDetectOutOfSupport)
{
  Eigen::Vector2d min(-3.0, 2.0);
  Eigen::Vector2d max(-2.0, 4.0);

  stats::Uniform d(min, max);

  EXPECT_TRUE(insupport(d, Eigen::Vector2d(-2.8, 3.3)));
  EXPECT_FALSE(insupport(d, Eigen::Vector2d(-1.0, 3.0)));
  EXPECT_FALSE(insupport(d, Eigen::Vector2d(-4.0, 3.0)));
  EXPECT_FALSE(insupport(d, Eigen::Vector2d(-2.5, 1.0)));
  EXPECT_FALSE(insupport(d, Eigen::Vector2d(-2.5, 5.0)));
}

TEST(UniformDistribution, pdfIsPositiveForValuesInSupport)
{
  Eigen::Vector2d min(-3.0, 1.0);
  Eigen::Vector2d max(2.0, 3.0);

  stats::Uniform d(min, max);

  EXPECT_GT(stats::pdf(d, Eigen::Vector2d(0.0, 2.0)), 0.0);
  EXPECT_DOUBLE_EQ(0.0, stats::pdf(d, Eigen::Vector2d(-4.0, 2.0)));
}

TEST(UniformDistribution, pdfIsConstant)
{
  Eigen::Vector2d min(-3.0, 1.0);
  Eigen::Vector2d max(2.0, 3.0);

  stats::Uniform d(min, max);

  double pdfAtMean = stats::pdf(d, stats::mean(d));
  EXPECT_DOUBLE_EQ(pdfAtMean, stats::pdf(d, Eigen::Vector2d(0.0, 2.0)));
  EXPECT_DOUBLE_EQ(pdfAtMean, stats::pdf(d, Eigen::Vector2d(-1.0, 2.5)));
  EXPECT_DOUBLE_EQ(pdfAtMean, stats::pdf(d, Eigen::Vector2d(-2.9, 2.9)));
  EXPECT_DOUBLE_EQ(pdfAtMean, stats::pdf(d, Eigen::Vector2d(1.9, 1.1)));
}
