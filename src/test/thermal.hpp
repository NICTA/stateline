/**
 * Contains common testing utility functions for world
 *
 * @file world.hpp
 * @author Nahid Akbar
 * @date 2014-06-04
 * @license General Public License version 3 or later
 * @copyright (c) 2013, NICTA
 */

#pragma once

#include "datatype/datatypes.hpp"
#include "test/common.hpp"

namespace obsidian
{

  bool operator==(const ThermalSpec& g, const ThermalSpec& p)
  {
    return (g.locations == p.locations) && (g.surfaceTemperature == p.surfaceTemperature)
        && (g.lowerBoundary == p.lowerBoundary) && (g.lowerBoundaryIsHeatFlow == p.lowerBoundaryIsHeatFlow)
        && (g.voxelisation == p.voxelisation) && (g.noise == p.noise);
  }

  bool operator==(const ThermalParams& g, const ThermalParams& p)
  {
    return (g.returnSensorData == p.returnSensorData);
  }

  bool operator==(const ThermalResults& g, const ThermalResults& p)
  {
    return (g.likelihood == p.likelihood) && (g.readings == p.readings);
  }

  inline std::ostream& operator<<(std::ostream& os, const ThermalSpec& spec)
  {
    os << "THERMAL SPEC" << "  LOCATIONS : " << spec.locations << " ST " << spec.surfaceTemperature << " LB "
        << spec.lowerBoundary << " LBHF " << spec.lowerBoundaryIsHeatFlow << " VX " << spec.voxelisation << " NOISE "
        << spec.noise << std::endl;
    return os;
  }

  inline std::ostream& operator<<(std::ostream& os, const ThermalResults& results)
  {
    os << "THERMAL RESULTS " << "  READINGS " << results.readings << " LIKELIHOOD " << results.likelihood << std::endl;
    return os;
  }

  template<>
  inline void generateVariations<ThermalSpec>(std::function<void(ThermalSpec)> test)
  {
    for (uint l :
    { 0, 1, 5, 20 })
    {
      for (bool fl :
      { true, false })
      {
        ThermalSpec spec;
        spec.locations = testing::randomMatrix(l, 3);
        spec.surfaceTemperature = testing::randomDouble();
        spec.lowerBoundary = testing::randomDouble();
        spec.lowerBoundaryIsHeatFlow = fl;
        spec.voxelisation = testing::randomVoxel();
        spec.noise = testing::randomNoise();
        test(spec);
      }
    }
  }
  template<>
  inline void generateVariations<ThermalParams>(std::function<void(ThermalParams)> test)
  {
    for (bool u :
    { true, false })
    {
      ThermalParams param;
      param.returnSensorData = u;
      test(param);
    }
  }
  template<>
  inline void generateVariations<ThermalResults>(std::function<void(ThermalResults)> test)
  {
    for (uint u :
    { 0, 5, 20 })
    {
      ThermalResults g;
      g.likelihood = testing::randomDouble();
      g.readings = testing::randomMatrix(u, 1);
      test(g);
    }
  }

}
