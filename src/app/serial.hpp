//!
//! Contains basic serialisation functions.
//!
//! \file app/serial.hpp
//! \author Darren Shen
//! \date 2014
//! \licence Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <string>
#include <Eigen/Dense>

namespace stateline
{
  inline std::string serialise(const Eigen::VectorXd &vector)
  {
    return std::string((char *)vector.data(), vector.size() * sizeof(double));
  }

  inline std::string serialise(const Eigen::MatrixXd &matrix)
  {
    std::uint32_t rows = matrix.rows();

    // Header contains the number of rows in the matrix
    std::string buffer = std::string((const char *)&rows, sizeof(std::uint32_t));
    return buffer + std::string((char *)matrix.data(), matrix.size() * sizeof(double));
  }

  template <class T>
  inline T unserialise(const std::string &str);

  template <>
  inline Eigen::VectorXd unserialise(const std::string &str)
  {
    Eigen::VectorXd vector(str.length() / sizeof(double));
    memcpy(vector.data(), str.c_str(), str.length());
    return vector;
  }

  template <>
  inline Eigen::MatrixXd unserialise(const std::string &str)
  {
    // Read the header containing the number of rows.
    std::size_t rows = *((std::uint32_t *)str.data());

    // Extract the actual matrix data
    std::string data = str.substr(sizeof(std::uint32_t));

    Eigen::MatrixXd matrix(rows, data.length() / rows / sizeof(double));
    memcpy(matrix.data(), data.c_str(), data.length());
    return matrix;
  }
}
