//!
//! Contains some useful mcmc default functions
//!
//! \file infer/datatypes.hpp
//! \author Lachlan McCalman
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "infer/datatypes.hpp"

namespace stateline
{
  namespace mcmc
  {
    std::vector<comms::JobData> singleJobConstruct(const Eigen::VectorXd &x);
    double singleJobEnergy(const std::vector<comms::ResultData> &results);
  }
}
