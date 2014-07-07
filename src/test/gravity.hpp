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

  inline bool operator==(const GravSpec& g, const GravSpec& p)
  {
    return (g.locations == p.locations) && (g.voxelisation == p.voxelisation) && (g.noise == p.noise);
  }

  inline bool operator==(const GravParams& g, const GravParams& p)
  {
    return (g.returnSensorData == p.returnSensorData);
  }

  inline bool operator==(const GravResults& g, const GravResults& p)
  {
    return (g.likelihood == p.likelihood) && (g.readings == p.readings);
  }

  inline std::ostream& operator<<(std::ostream& os, const GravSpec& spec)
  {
    os << "GRAVITY SPEC" << "  LOCATIONS : " << spec.locations << " VX " << spec.voxelisation << " NOISE " << spec.noise
        << std::endl;
    return os;
  }

  inline std::ostream& operator<<(std::ostream& os, const GravResults& results)
  {
    os << "GRAVITY RESULTS " << "  READINGS " << results.readings << " LIKELIHOOD " << results.likelihood << std::endl;
    return os;
  }

  template<>
  inline void generateVariations<GravSpec>(std::function<void(GravSpec)> test)
  {
    for (uint l :
    { 0, 1, 5, 20 })
    {
      GravSpec spec;
      spec.locations = testing::randomMatrix(l, 3);
      spec.voxelisation = testing::randomVoxel();
      spec.noise = testing::randomNoise();
      test(spec);
    }
  }
  template<>
  inline void generateVariations<GravParams>(std::function<void(GravParams)> test)
  {
    for (bool u :
    { true, false })
    {
      GravParams param;
      param.returnSensorData = u;
      test(param);
    }
  }
  template<>
  inline void generateVariations<GravResults>(std::function<void(GravResults)> test)
  {
    for (uint u :
    { 0, 5, 20 })
    {
      GravResults g;
      g.likelihood = testing::randomDouble();
      g.readings = testing::randomMatrix(u, 1);
      test(g);
    }
  }

}
