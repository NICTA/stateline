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

#include "test/common.hpp"
#include "prior/prior.hpp"

namespace obsidian
{
  namespace testing
  {
    template<typename OFST, typename CPT, typename RPT>
    void initWorld(WorldSpec & outSpec, WorldParams & outParams, double mX, double MX, uint xres, double mY, double MY, uint yres,
                   double mZ, double MZ, uint zres, OFST offset, CPT controlPoint, RPT rockProperty)
    {
      WorldSpec spec(mX, MX, mY, MY, mZ, MZ);
      WorldParams params;

      for (uint boundary = 0; boundary < zres; boundary++)
      {
        Eigen::MatrixXd controlPoints = Eigen::MatrixXd::Zero(xres, yres);
        Eigen::MatrixXd offsets = Eigen::MatrixXd::Zero(xres, yres);
        Eigen::VectorXd rockProperties = Eigen::VectorXd::Zero(static_cast<uint>(RockProperty::Count));
        for (uint x = 0; x < xres; x++)
        {
          for (uint y = 0; y < yres; y++)
          {
            double px = mX + (x * (MX - mX) / xres);
            double py = mY + (y * (MY - mY) / yres);
            controlPoints(x, y) = controlPoint(px, py, boundary);
            offsets(x, y) = offset(px, py, boundary);
          }
        }

        for (uint p = 0; p < static_cast<uint>(RockProperty::Count); p++)
        {
          rockProperties(p) = rockProperty(boundary, p);
        }

        BoundarySpec bspec { offsets, std::make_pair(xres, yres), BoundaryClass::Normal };
        spec.boundaries.push_back(bspec);

        params.controlPoints.push_back(controlPoints);
        params.rockProperties.push_back(rockProperties);
      }

      spec.boundariesAreTimes = false;

      outSpec = spec;
      outParams = params;
    }

  }

  /**
   * outputs world spec object (for debugging)
   */
  inline std::ostream& operator<<(std::ostream& os, const WorldSpec& spec)
  {
    os << "WORLD SPEC";
    os << "  Bounds X: " << spec.xBounds << ", Y: " << spec.yBounds << ", Z: " << spec.zBounds << std::endl;
    for (uint b = 0; b < spec.boundaries.size(); b++)
    {
      os << "  Boundary " << (spec.boundaries[b].boundaryClass == BoundaryClass::Normal ? "Normal" : "Warped") << " " << b << " Res: "
          << spec.boundaries[b].ctrlPointResolution << " Offset: " << std::endl;
      os << spec.boundaries[b].offset << std::endl;
    }
    return os;
  }

  /**
   * outputs world params object (for debugging)
   */
  inline std::ostream& operator<<(std::ostream& os, const WorldParams& params)
  {
    os << "WORLD PARAMS" << std::endl;
    os << "  PROPERTIES: " << params.rockProperties << std::endl;
    os << "  CONTROL POINTS: " << params.controlPoints << std::endl;
    return os;
  }

  inline bool operator==(const BoundarySpec& g, const BoundarySpec& p)
  {
    return (g.offset == p.offset) && (g.ctrlPointResolution == p.ctrlPointResolution) && (g.boundaryClass == p.boundaryClass);
  }

  inline bool operator==(const WorldSpec& g, const WorldSpec& p)
  {
    return (g.xBounds == p.xBounds) && (g.yBounds == p.yBounds) && (g.zBounds == p.zBounds) && (g.boundaries == p.boundaries)
        && (g.boundariesAreTimes == p.boundariesAreTimes);
  }

  namespace distrib
  {
    inline bool operator==(const MultiGaussian& g, const MultiGaussian& p)
    {
      return true;
//      return (g.mu == p.mu) && (g.sigma == p.sigma);
    }
  }

  namespace prior
  {
    inline bool operator==(const WorldParamsPrior& g, const WorldParamsPrior& p)
    {
//      std::cout << "XX" << (g.propertyPrior == p.propertyPrior) << (g.ctrlptMasks == p.ctrlptMasks) << (g.ctrlptMins == p.ctrlptMins)
//          << (g.ctrlptMaxs == p.ctrlptMaxs) << (g.ctrlptPrior == p.ctrlptPrior) << (g.propMasks == p.propMasks)
//          << (g.propMins == p.propMins) << (g.propMaxs == p.propMaxs) << std::endl;
//
      return (g.propertyPrior == p.propertyPrior) && (g.ctrlptMasks == p.ctrlptMasks) && (g.ctrlptMins == p.ctrlptMins)
          && (g.ctrlptMaxs == p.ctrlptMaxs) && (g.ctrlptPrior == p.ctrlptPrior) && (g.propMasks == p.propMasks)
          && (g.propMins == p.propMins) && (g.propMaxs == p.propMaxs);
//      return g.ctrlptMins == p.ctrlptMins;
    }
    template<typename T>
    std::ostream& operator<<(std::ostream& os, const std::vector<T>& spec)
    {
      os << "[";
      for (uint i = 0; i < spec.size(); i++)
      {
        os << spec[i];
        if (i != spec.size() - 1)
        {
          os << ", ";
        }
      }
      os << "]";
      return os;
    }
    inline std::ostream& operator<<(std::ostream& os, const WorldParamsPrior& p)
    {
      os << "WORLD PARAMS PRIOR" << std::endl;
//      os << p.ctrlptMins << std::endl;
      //      os << "  CONTROL POINTS: " << params.controlPoints << std::endl;
      return os;
    }

  }

  inline bool operator==(const WorldParams& g, const WorldParams& p)
  {
    return (g.rockProperties == p.rockProperties) && (g.controlPoints == p.controlPoints);
  }

  template<>
  inline void generateVariations<WorldSpec>(std::function<void(WorldSpec)> test)
  {
    for (uint brt : { true, false })
    {
      for (uint b : { 0, 1, 2 })
      {
        for (uint r : { 1, 2, 5 })
        {
          for (uint s : { 1, 2, 4 })
          {
            for (BoundaryClass c : { BoundaryClass::Normal, BoundaryClass::Warped })
            {
              WorldSpec spec;
              spec.xBounds = std::make_pair(testing::randomDouble(), testing::randomDouble());
              spec.yBounds = std::make_pair(testing::randomDouble(), testing::randomDouble());
              spec.zBounds = std::make_pair(testing::randomDouble(), testing::randomDouble());
              for (uint i = 0; i < b; i++)
              {
                BoundarySpec bspec;
                bspec.offset = testing::randomMatrix(r, r);
                bspec.ctrlPointResolution = std::make_pair(i % 2 == 0 ? r : s, i % 2 == 0 ? r : s);
                bspec.boundaryClass = c;
                spec.boundaries.push_back(bspec);
              }
              spec.boundariesAreTimes = brt;
              test(spec);
            }
          }
        }
      }
    }
  }

  template<>
  inline void generateVariations<WorldParams>(std::function<void(WorldParams)> test)
  {
    for (uint b : { 0, 1, 4 })
    {
      for (uint rpr : { 1, 4 })
      {
        for (uint cpr : { 1, 4 })
        {
          WorldParams param;
          for (uint i = 0; i < b; i++)
          {
            param.rockProperties.push_back(testing::randomMatrix(rpr, 1));
          }
          for (uint i = 0; i < b; i++)
          {
            param.controlPoints.push_back(testing::randomMatrix(cpr, cpr));
          }
          test(param);
        }
      }
    }
  }

  template<>
  inline void generateVariations<prior::WorldParamsPrior>(std::function<void(prior::WorldParamsPrior)> test)
  {
    for (uint l : { 1, 5 })
    {
      for (uint c : { 1, 5})
      {
        std::vector<distrib::MultiGaussian> ctrlpts;
        std::vector<Eigen::MatrixXi> ctrlptMasks;
        std::vector<Eigen::MatrixXd> ctrlptMins;
        std::vector<Eigen::MatrixXd> ctrlptMaxs;
        std::vector<distrib::MultiGaussian> properties;
        std::vector<Eigen::VectorXi> propMasks;
        std::vector<Eigen::VectorXd> propMins;
        std::vector<Eigen::VectorXd> propMaxs;
        std::vector<BoundaryClass> classes;
        uint np = static_cast<uint>(RockProperty::Count);
        Eigen::VectorXd coupledSds = testing::randomMatrix(l, 1);
        Eigen::VectorXd uncoupledSds = testing::randomMatrix(l, 1);
        for (uint i = 0; i < l; i++)
        {
          ctrlpts.push_back(distrib::coupledGaussianBlock(Eigen::MatrixXd::Zero(c, c), 0, 0));
          ctrlptMasks.push_back(Eigen::MatrixXi::Ones(c, c));
          ctrlptMins.push_back(Eigen::MatrixXd::Ones(c, c) * -std::numeric_limits<double>::max()/10);
          ctrlptMaxs.push_back(Eigen::MatrixXd::Ones(c, c) * std::numeric_limits<double>::max()/10);
          properties.push_back(distrib::MultiGaussian(Eigen::VectorXd::Ones(np), Eigen::MatrixXd::Ones(np, np)));
          propMasks.push_back(Eigen::MatrixXi::Ones(np, 1));
          propMins.push_back(Eigen::MatrixXd::Ones(np, 1) * -std::numeric_limits<double>::max()/10);
          propMaxs.push_back(Eigen::MatrixXd::Ones(np, 1) * std::numeric_limits<double>::max()/10);
          classes.push_back(BoundaryClass::Normal);
        }
        prior::WorldParamsPrior prior(ctrlpts, ctrlptMasks, ctrlptMins, ctrlptMaxs, coupledSds, uncoupledSds, properties, propMasks,
                                      propMins, propMaxs, classes);
        test(prior);
      }
    }
  }
}
