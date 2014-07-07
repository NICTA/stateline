/**
 * Contains common testing utility functions
 *
 * @file common.hpp
 * @author Nahid Akbar
 * @date 2014-06-03
 * @license General Public License version 3 or later
 * @copyright (c) 2013, NICTA
 */

#pragma once

#include <Eigen/Core>
#include <iostream>
#include <gtest/gtest.h>
#include "datatype/datatypes.hpp"

namespace Eigen
{
  template<typename T, int C, int R>
  bool operator==(const Matrix<T, C, R> g, const Matrix<T, C, R> p)
  {
    uint gr = g.rows();
    uint gc = g.cols();
    uint pr = p.rows();
    uint pc = p.cols();
    if (gr == 0 || gc == 0)
      gr = gc = 0;
    if (pr == 0 || pc == 0)
      pr = pc = 0;
    if (gr == pr && gc == pc)
    {
      for (uint i = 0; i < gr; i++)
      {
        for (uint j = 0; j < gc; j++)
        {
          if (!(g(i, j) == p(i, j)))
          {
            std::cout << __FUNCTION__ << "elem" << i << "," << j << " fail" << std::endl;
            return false;
          }
        }
      }
//      std::cout << "all is good" << std::endl;
      return true;
    }
    std::cout << __FUNCTION__ << "dims don't match " << gr << gc << pr << pc << std::endl;
    return false;
  }
}

namespace obsidian
{
  namespace testing
  {
    double randomDouble();
    uint randomUint();
    std::complex<double> randomComplex();
    Eigen::MatrixXd randomMatrix(int rows, int cols);
    std::vector<Eigen::VectorXi> randomNVecXi(int items, int vdim);
    std::vector<Eigen::VectorXd> randomNVecXd(int items, int vdim);

    inline NoiseSpec randomNoise()
    {
      return
      { testing::randomDouble(), testing::randomDouble()};
    }
    inline VoxelSpec randomVoxel()
    {
      return
      { testing::randomUint(), testing::randomUint(), testing::randomUint(), testing::randomUint()};
    }
  }
  /**
   * outputs a pair
   */
  template<typename A, typename B>
  std::ostream& operator<<(std::ostream& os, const std::pair<A, B>& spec)
  {
    os << "(" << spec.first << ", " << spec.second << ")";
    return os;
  }
  /**
   * outputs a vector
   */
  template<typename T>
  std::ostream& operator<<(std::ostream& os, const std::vector<T>& spec)
  {
    os << "[";
    for (uint i = 0; i < spec.size(); i++)
    {
      os << spec[i];
      if (i != spec.size() - 1)
      {
        os << ", ";
      }
    }
    os << "]";
    return os;
  }

  inline std::ostream& operator<<(std::ostream& os, const NoiseSpec& spec)
  {
    os << "INVERSE GAMMA " << spec.inverseGammaAlpha << ", " << spec.inverseGammaBeta << std::endl;
    return os;
  }

  inline std::ostream& operator<<(std::ostream& os, const VoxelSpec& spec)
  {
    os << "VOXEL SPEC " << spec.xResolution << ", " << spec.yResolution << ", " << spec.zResolution << ", "
        << spec.supersample << std::endl;
    return os;
  }

  template<typename T>
  bool operator==(const std::vector<T>& g, const std::vector<T>& p)
  {
    if (g.size() != p.size())
    {
      std::cout << __FUNCTION__ << "size mismatch " << g.size() << "," << p.size() << std::endl;
//      std::cout << __FUNCTION__ << "size mismatch " << g << "," << p << std::endl;
      return false;
    }
    for (size_t i = 0; i < g.size(); i++)
    {
      if (!(g[i] == p[i]))
      {
        std::cout << __FUNCTION__ << "element " << i << " mismatch" << std::endl;
        return false;
      }
    }
//    std::cout << "all is good" << std::endl;
    return true;
  }

  inline bool operator==(const NoiseSpec& g, const NoiseSpec& p)
  {
    return (g.inverseGammaAlpha == p.inverseGammaAlpha) && (g.inverseGammaAlpha == p.inverseGammaAlpha);
  }
  inline bool operator==(const VoxelSpec& g, const VoxelSpec& p)
  {
    return (g.xResolution == p.xResolution) && (g.yResolution == p.yResolution) && (g.zResolution == p.zResolution)
        && (g.supersample == p.supersample);
  }

  template<typename T>
  void generateVariations(std::function<void(T)> test);
}
