//!
//! Contains the interface for the uniform distribution.
//!
//! \file stats/uniform.hpp
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "multivariate.hpp"

namespace stateline
{
  namespace stats
  {
    class Uniform : public Multivariate
    {
      public:
        //! Create a uniform distribution.
        //!
        //! \param min The lowerbound of the support in each dimension. The PDF
        //!            of the distribution is zero for values below the min.
        //! \param max The upperbound of the support in each dimension. The PDF
        //!            of the distribution is zero for values above the max.
        //!
        Uniform(const Eigen::VectorXd &min, const Eigen::VectorXd &max);

        Eigen::VectorXd min() const;
        Eigen::VectorXd max() const;

      private:
        Eigen::VectorXd min_;
        Eigen::VectorXd max_;
    };

    template <>
    Eigen::VectorXd mean(const Uniform &d);

    template <>
    Eigen::VectorXd var(const Uniform &d);

    template <>
    bool insupport(const Uniform &d, const Eigen::VectorXd &x);

    template <>
    double logpdf(const Uniform &d, const Eigen::VectorXd &x);

    template <class RNG>
    Eigen::VectorXd sample(const Uniform &d, RNG &rng);
  }
}
