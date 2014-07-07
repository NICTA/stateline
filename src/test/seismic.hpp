//!
//! Contains common testing utility functions for world
//
//! \file test/seismic.hpp
//! \author Nahid Akbar
//! \date 2014
//! \license Affero General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#pragma once

#include "datatype/datatypes.hpp"
#include "test/common.hpp"

namespace obsidian
{
  bool operator==(const Seismic1dSpec& g, const Seismic1dSpec& p)
  {
    return (g.locations == p.locations) && (g.interfaces == p.interfaces) && (g.noise == p.noise);
  }

  bool operator==(const Seismic1dParams& g, const Seismic1dParams& p)
  {
    return (g.returnSensorData == p.returnSensorData);
  }

  bool operator==(const Seismic1dResults& g, const Seismic1dResults& p)
  {
    return (g.likelihood == p.likelihood) && (g.readings == p.readings);
  }

  inline std::ostream& operator<<(std::ostream& os, const Seismic1dSpec& spec)
  {
    os << "SEISMIC1D SPEC LOCATIONS" << std::endl << spec.locations << std::endl
       << " INTERFACCES " << spec.interfaces << std::endl;
    return os;
  }

  inline std::ostream& operator<<(std::ostream& os, const Seismic1dResults& results)
  {
    os << "SEISMIC 1D RESULTS "<< "  READINGS " << results.readings
       << " LIKELIHOOD " << results.likelihood << std::endl;
    return os;
  }

  template<>
  inline void generateVariations<Seismic1dSpec>(std::function<void(Seismic1dSpec)> test)
  {
    for (uint u : { 0, 1, 5, 20 })
    {
      for (uint v : { 1, 5, 20 })
      {
        Seismic1dSpec spec;
        spec.locations = testing::randomMatrix(u, 2);
        spec.interfaces = testing::randomNVecXi(u, v);
        spec.noise = testing::randomNoise();
        test(spec);
      }
    }
  }

  template<>
  inline void generateVariations<Seismic1dParams>(std::function<void(Seismic1dParams)> test)
  {
    for (bool u : { true, false })
    {
      Seismic1dParams param;
      param.returnSensorData = u;
      test(param);
    }
  }

  template<>
  inline void generateVariations<Seismic1dResults>(std::function<void(Seismic1dResults)> test)
  {
    for (uint u : { 0, 5, 20 })
    {
      for (uint v : { 1, 5, 20 })
      {
        Seismic1dResults g;
        g.likelihood = testing::randomDouble();
        g.readings = testing::randomNVecXd(u, v);
        test(g);
      }
    }
  }
}
