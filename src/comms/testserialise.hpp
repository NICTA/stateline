//!
//! \file comms/testserialise.cpp
//! \author Lachlan McCalman
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include <glog/logging.h>
#include "gtest/gtest.h"

#include "comms/messages.hpp"
#include "serial/serial.hpp"
#include <Eigen/Core>

namespace obsidian
{
  bool operator==(const GravResults& g, const GravResults& p)
  {
    return (g.likelihood == p.likelihood) && (g.readings == p.readings);
  }
}
using namespace obsidian;
using namespace obsidian::comms;
namespace stateline
{

  namespace comms
  {

    class Serialise: public testing::Test
    {
    public:
      Eigen::MatrixXd matrix_;
      Eigen::VectorXd vector_;
      std::string matrixString_;
      std::string vectorString_;
      std::vector<char> matrixChars_;
      std::vector<char> vectorChars_;
      GravResults gravResults_;

      virtual void SetUp()
      {
        matrix_ = Eigen::MatrixXd::Zero(3, 2);
        matrix_(0, 0) = 1.0;
        matrix_(1, 1) = 2.0;
        vector_ = Eigen::VectorXd::Zero(4);
        vector_(0) = 1.0;
        vector_(1) = 2.0;
        vector_(2) = 3.0;
        vector_(3) = 4.0;
        matrixChars_ =
        { '\0', '\0', '\0', '\0', '\0', '\0', '\xF0', 0x3F,
          '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
          '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
          '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
          '\0', '\0', '\0', '\0', '\0', '\0', '\0', 0x40,
          '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
        vectorChars_ =
        { '\0', '\0', '\0', '\0', '\0', '\0', '\xF0', 0x3F,
          '\0', '\0', '\0', '\0', '\0', '\0', '\0', 0x40,
          '\0', '\0', '\0', '\0', '\0', '\0', '\b', 0x40,
          '\0', '\0', '\0', '\0', '\0', '\0', 0x10, 0x40};
        matrixString_ = std::string(matrixChars_.begin(), matrixChars_.end());
        vectorString_ = std::string(vectorChars_.begin(), vectorChars_.end());

        gravResults_.readings = Eigen::VectorXd::Random(10);
      }
      virtual void TearDown()
      {
      }
    };

  TEST_F(Serialise, matrixToString)
  {
    std::string s = matrixString(matrix_);
    EXPECT_EQ(matrixString_, s);
  }

  TEST_F(Serialise, vectorToString)
  {
    std::string s = vectorString(vector_);
    EXPECT_EQ(vectorString_, s);
  }

  TEST_F(Serialise, stringToMatrix)
  {
    Eigen::MatrixXd m = stringMatrix(matrixString_, matrix_.rows());
    EXPECT_EQ(m, matrix_);
  }

  TEST_F(Serialise, stringToVector)
  {
    Eigen::VectorXd v = stringVector(vectorString_);
    EXPECT_EQ(v, vector_);
  }

  TEST_F(Serialise, vector)
  {
    std::string s = vectorString(vector_);
    Eigen::VectorXd v = stringVector(s);
    EXPECT_EQ(v, vector_);
  }

  TEST_F(Serialise, matrix)
  {
    std::string s = matrixString(matrix_);
    Eigen::MatrixXd m = stringMatrix(s, matrix_.rows());
    EXPECT_EQ(m, matrix_);
  }

// already tested in serial
//    TEST_F(Serialise, gravResults)
//    {
//      std::string s =  serialise(gravResults_);
//      GravResults p;
//      unserialise(s, p);
//      EXPECT_EQ(gravResults_, p);
//    }

}
 // namespace comms
}// namespace obsidian
