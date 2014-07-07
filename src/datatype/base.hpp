//!
//! Contains common type header includes.
//!
//! \file datatype/base.hpp
//! \author Nahid Akbar
//! \date June 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <utility>
#include <complex>
#include <cctype>

template<typename T>
inline bool is_neg_infinity(T val)
{
  return std::isinf(val) && std::signbit(val);
}

#include <Eigen/Core>
#include <Eigen/Dense>
