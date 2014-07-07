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

  bool operator==(const MagSpec& g, const MagSpec& p)
  {
    return (g.locations == p.locations) && (g.voxelisation == p.voxelisation) && (g.noise == p.noise)
        && (g.backgroundField == p.backgroundField);
  }

  bool operator==(const MagParams& g, const MagParams& p)
  {
    return (g.returnSensorData == p.returnSensorData);
  }

  bool operator==(const MagResults& g, const MagResults& p)
  {
    return (g.likelihood == p.likelihood) && (g.readings == p.readings);
  }

  inline std::ostream& operator<<(std::ostream& os, const MagSpec& spec)
  {
    os << "MAGNETISM SPEC" << "  LOCATIONS : " << spec.locations << " VX " << spec.voxelisation << " NOISE "
        << spec.noise << " FIELD " << spec.backgroundField << std::endl;
    return os;
  }

  inline std::ostream& operator<<(std::ostream& os, const MagResults& results)
  {
    os << "MAGNETISM RESULTS " << "  READINGS " << results.readings << " LIKELIHOOD " << results.likelihood
        << std::endl;
    return os;
  }

  template<>
  inline void generateVariations<MagSpec>(std::function<void(MagSpec)> test)
  {
    for (uint l :
    { 0, 1, 5, 20 })
    {
      MagSpec spec;
      spec.locations = testing::randomMatrix(l, 3);
      spec.voxelisation = testing::randomVoxel();
      spec.noise = testing::randomNoise();
      spec.backgroundField = testing::randomMatrix(3, 1);
      test(spec);
    }
  }
  template<>
  inline void generateVariations<MagParams>(std::function<void(MagParams)> test)
  {
    for (bool u :
    { true, false })
    {
      MagParams param;
      param.returnSensorData = u;
      test(param);
    }
  }
  template<>
  inline void generateVariations<MagResults>(std::function<void(MagResults)> test)
  {
    for (uint u :
    { 0, 5, 20 })
    {
      MagResults g;
      g.likelihood = testing::randomDouble();
      g.readings = testing::randomMatrix(u, 1);
      test(g);
    }
  }

}
