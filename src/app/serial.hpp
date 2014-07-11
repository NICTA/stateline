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
  std::string serialise(const Eigen::VectorXd &vector)
  {
    return std::string((char *)vector.data(), vector.size() * sizeof(double));
  }

  template <class T>
  T unserialise(const std::string &str);

  template <>
  Eigen::VectorXd unserialise(const std::string &str)
  {
    Eigen::VectorXd vector(str.length() / sizeof(double));
    memcpy(vector.data(), str.c_str(), str.length());
    return vector;
  }
}
