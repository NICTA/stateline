//!
//! Contains the interface for mixture models.
//!
//! \file stats/mixture.hpp
//! \author Darren Shen
//! \date 2014
//! \license Lesser General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "normal.hpp"

namespace stateline
{
  namespace stats
  {
    template <class Distribution>
    class Mixture : public Multivariate
    {
      public:
        typedef typename std::vector<Distribution>::const_iterator iterator;
        typedef Distribution Component;

        template <class Container>
        Mixture(const Container &components)
          : Mixture(Eigen::VectorXd::Ones(stats::length(components[0])), components)
        {
        }

        template <class Container>
        Mixture(const Eigen::VectorXd weights, const Container &components)
          : Multivariate(weights.size()),
            weights_(weights),
            components_(std::begin(components), std::end(components))
        {
          // Ensure that all the components have the same number of dimensions
          for (const auto &comp : components)
          {
            assert((uint)weights.size() == stats::length(comp));
          }
        }

        Eigen::VectorXd weights() const
        {
          return weights_;
        }

        iterator begin() const
        {
          return components_.begin();
        }

        iterator end() const
        {
          return components_.end();
        }

      private:
        Eigen::VectorXd weights_;
        std::vector<Distribution> components_;
    };

    //! Useful typedefs for common mixtures
    using GaussianMixture = Mixture<Normal>; 

    template <class Distribution>
    double pdf(const Mixture<Distribution> &d, const Eigen::VectorXd &x)
    {
      double wsum = 0;
      Eigen::VectorXd::Index i = 0;
      for (const auto &comp : d)
      {
        wsum += d.weights()(i) * pdf(comp, x);
      }
      return wsum;
    }
  }
}
