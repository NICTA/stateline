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

  inline bool operator==(const MtAnisoSpec& g, const MtAnisoSpec& p)
  {
    return (g.locations == p.locations) && (g.freqs == p.freqs) &&
    /*(g.likelihoodType == p.likelihoodType) && */
    (g.noise == p.noise) && (g.ignoreAniso == p.ignoreAniso);
  }

  inline bool operator==(const MtAnisoParams& g, const MtAnisoParams& p)
  {
    return (g.returnSensorData == p.returnSensorData);
  }

  inline bool operator==(const MtAnisoResults& g, const MtAnisoResults& p)
  {
    return (g.readings == p.readings) && (g.likelihood == p.likelihood);
  }

  inline std::ostream& operator<<(std::ostream& os, const MtAnisoSpec& spec)
  {
    os << "MT ANISO SPEC";
    os << "  LOCATIONS : " << spec.locations << " FREQS " << spec.freqs << " NOISE " << spec.noise << " IGNANSIO "
        << spec.ignoreAniso << std::endl;
    return os;
  }

  inline std::ostream& operator<<(std::ostream& os, const MtAnisoResults& results)
  {
    os << "MT ANISO RESULTS ";
    os << "  READINGS " << results.readings << std::endl;
    return os;
  }

  template<>
  inline void generateVariations<MtAnisoSpec>(std::function<void(MtAnisoSpec)> test)
  {
    for (uint l :
    { 1, 5, 50 })
    {
      for (uint f :
      { 1, 5, 20 })
      {
        for (bool ig :
        { true, false })
        {
          MtAnisoSpec g;
          g.locations = testing::randomMatrix(l, 3);
          g.freqs = testing::randomNVecXd(l, f);
          g.noise = testing::randomNoise();
          g.ignoreAniso = ig;
          test(g);
        }
      }
    }
  }
  template<>
  inline void generateVariations<MtAnisoParams>(std::function<void(MtAnisoParams)> test)
  {
    for (bool u :
    { true, false })
    {
      MtAnisoParams param;
      param.returnSensorData = u;
      test(param);
    }
  }
  template<>
  inline void generateVariations<MtAnisoResults>(std::function<void(MtAnisoResults)> test)
  {
    for (uint l :
    { 0, 1, 5, 50 })
    {
      for (uint f :
      { 1, 5, 50 })
      {
        MtAnisoResults g;
        g.readings.resize(l);
        g.phaseTensor.resize(l);
        g.alpha.resize(l);
        g.beta.resize(l);
        for (uint i = 0; i < l; i++)
        {
          g.readings[i].resize(f, 4);
          g.phaseTensor[i].resize(f, 4);
          g.alpha[i].resize(f);
          g.beta[i].resize(f);
          for (uint j = 0; j < f; j++)
          {
            for (uint k = 0; k < 4; k++)
            {
              g.readings[i](j, k) = testing::randomComplex();
              g.phaseTensor[i](j, k) = testing::randomDouble();
            }
            g.alpha[i](j) = testing::randomDouble();
            g.beta[i](j) = testing::randomDouble();
          }
        }
        test(g);
      }
    }
  }

}
