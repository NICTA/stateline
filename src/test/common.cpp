/**
 * Contains common testing utility functions
 *
 * @file common.hpp
 * @author Nahid Akbar
 * @date 2014-06-03
 * @license General Public License version 3 or later
 * @copyright (c) 2013, NICTA
 */

#include "common.hpp"
#include <random>

namespace obsidian
{
  namespace testing
  {
    // get some random number generators going
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> d(0, 5);

    double randomDouble()
    {
//      if (d(gen) > 10)
//        return std::numeric_limits<double>::infinity();
//      if (d(gen) < -10)
//        return -std::numeric_limits<double>::infinity();
      return d(gen);
    }
    uint randomUint()
    {
//      if (d(gen) > 10)
//        return std::numeric_limits<double>::infinity();
//      if (d(gen) < -10)
//        return -std::numeric_limits<double>::infinity();
      return static_cast<uint>(d(gen)) % 5000;
    }
    std::complex<double> randomComplex()
    {
      return std::complex<double>(randomDouble(), randomDouble());
    }

    Eigen::MatrixXd randomMatrix(int rows, int cols)
    {
      if (rows > 0 and cols > 0)
      {
        Eigen::MatrixXd ret(rows, cols);
        for (uint i = 0; i < ret.rows(); i++)
        {
          for (uint j = 0; j < ret.cols(); j++)
          {
            ret(i, j) = d(gen);
          }
        }
        return ret;
      } else
      {
        return Eigen::MatrixXd(rows, cols);
      }
    }
    std::vector<Eigen::VectorXi> randomNVecXi(int items, int vdim)
    {
      std::vector < Eigen::VectorXi > ret;
      ret.resize(items);
      for (uint i = 0; i < ret.size(); i++)
      {
        ret[i].resize(vdim);
        for (int j = 0; j < vdim; j++)
        {
          ret[i](j) = d(gen);
        }

      }
      return ret;
    }
    std::vector<Eigen::VectorXd> randomNVecXd(int items, int vdim)
    {
      std::vector < Eigen::VectorXd > ret;
      ret.resize(items);
      for (uint i = 0; i < ret.size(); i++)
      {
        ret[i].resize(vdim);
        for (int j = 0; j < vdim; j++)
        {
          ret[i](j) = randomDouble();
        }
      }
      return ret;
    }
  }
}
