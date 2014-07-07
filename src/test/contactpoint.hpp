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

  bool operator==(const ContactPointSpec& g, const ContactPointSpec& p)
  {
    return (g.locations == p.locations) && (g.interfaces == p.interfaces) && (g.noise == p.noise);
  }

  bool operator==(const ContactPointParams& g, const ContactPointParams& p)
  {
    return (g.returnSensorData == p.returnSensorData);
  }

  bool operator==(const ContactPointResults& g, const ContactPointResults& p)
  {
    return (g.likelihood == p.likelihood) && (g.readings == p.readings);
  }

  inline std::ostream& operator<<(std::ostream& os, const ContactPointSpec& spec)
  {
    os << "Contact Point SPEC" << "  LOCATIONS : " << spec.locations << " INTERFACES " << spec.interfaces << " NOISE "
        << spec.noise << std::endl;
    return os;
  }

  inline std::ostream& operator<<(std::ostream& os, const ContactPointResults& results)
  {
    os << "Contact Point RESULTS "<< "  READINGS " << results.readings << " LIKELIHOOD " << results.likelihood << std::endl;
    return os;
  }

  template<>
  inline void generateVariations<ContactPointSpec>(std::function<void(ContactPointSpec)> test)
  {
    for (uint u :
    { 0, 1, 5, 20 })
    {
      for (uint v :
      { 1, 1, 5, 20 })
      {
        ContactPointSpec spec;
        spec.locations = testing::randomMatrix(u, 2);
        spec.interfaces = testing::randomNVecXi(u, v);
        spec.noise = testing::randomNoise();
        test(spec);
      }
    }
  }

  template<>
  inline void generateVariations<ContactPointParams>(std::function<void(ContactPointParams)> test)
  {
    for (bool u :
    { true, false })
    {
      ContactPointParams param;
      param.returnSensorData = u;
      test(param);
    }
  }

  template<>
  inline void generateVariations<ContactPointResults>(std::function<void(ContactPointResults)> test)
  {
    for (uint u :
    { 0, 5, 20 })
    {
      for (uint v :
      { 1, 5, 20 })
      {
        ContactPointResults result;
        result.likelihood = testing::randomDouble();
        result.readings = testing::randomNVecXd(u, v);
        test(result);
      }
    }
  }
}

