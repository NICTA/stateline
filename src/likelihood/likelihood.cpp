//!
//! \file likelihood/likelihood.cpp
//! \author Darren Shen
//! \date May 2014
//! \license General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "likelihood.hpp"

#include <glog/logging.h>
#include <cmath>

namespace obsidian
{
  namespace lh
  {
    double stdDev(const Eigen::VectorXd& x)
    {
      return std::sqrt((x.array() - x.mean()).pow(2.0).sum() / (double) (x.size()));
    }

    double stdDev(const std::vector<Eigen::VectorXd>& x)
    {
      uint totalSize = 0;
      for (auto const& i : x)
        totalSize += i.size();
      Eigen::VectorXd full(totalSize);
      uint start = 0;
      for (auto const& i : x)
      {
        full.segment(start, i.size()) = i;
        start += i.size();
      }
      return stdDev(full);
    }

    double gaussian(const Eigen::VectorXd &real, const Eigen::VectorXd &candidate, double sensorSd)
    {
      Eigen::VectorXd delta = real - candidate;
      return -delta.squaredNorm() / (2 * sensorSd * sensorSd) - 0.5 * std::log(6.28318530718 * sensorSd * sensorSd) * delta.rows();
    }

    double normalInverseGamma(const Eigen::VectorXd &real, const Eigen::VectorXd &candidate, double A, double B)
    {
      Eigen::VectorXd delta = real - candidate;

      double norm = std::lgamma(A + 0.5) - std::lgamma(A) - 0.5 * std::log(6.28318530718) + std::log(B) * A;
      return (-(A + 0.5) * (B + 0.5 * delta.array().square()).log() + norm).sum();
    }

    template<>
    double likelihood<ForwardModel::GRAVITY>(const GravResults& synthetic, const GravResults& real, const GravSpec& spec)
    {
      // We shift both readings to have mean zero (no-one gets the dc offsets
      // right)
      if (real.readings.size() == 0)
        return 0.0;
      Eigen::VectorXd synShifted = synthetic.readings.array() - synthetic.readings.mean();
      Eigen::VectorXd realShifted = real.readings.array() - real.readings.mean();
      double sigma = stdDev(realShifted);
      VLOG(3) << "gravity likelihood sigma: " << sigma;
      double l = lh::normalInverseGamma(synShifted / sigma, realShifted / sigma, spec.noise.inverseGammaAlpha, spec.noise.inverseGammaBeta);
      VLOG(2) << "Gravity Likelihood: " << l;
      return l;
    }

    template<>
    double likelihood<ForwardModel::MAGNETICS>(const MagResults& synthetic, const MagResults& real, const MagSpec& spec)
    {
      // We shift both readings to have mean zero (no-one gets the dc offsets
      // right)
      if (real.readings.size() == 0)
        return 0.0;
      Eigen::VectorXd synShifted = synthetic.readings.array() - synthetic.readings.mean();
      Eigen::VectorXd realShifted = real.readings.array() - real.readings.mean();
      double sigma = stdDev(realShifted);
      VLOG(3) << "magnetic likelihood sigma: " << sigma;
      double l = lh::normalInverseGamma(synShifted / sigma, realShifted / sigma, spec.noise.inverseGammaAlpha, spec.noise.inverseGammaBeta);
      VLOG(2) << "Magnetics Likelihood: " << l;
      return l;
    }

    Eigen::VectorXd mtLikelihoodVector(const Eigen::MatrixX4cd& impedences)
    {
      Eigen::VectorXd v(2 * 4 * impedences.rows());
      for (uint rowID = 0; rowID < impedences.rows(); rowID++)
      {
        v(rowID * 8 + 0) = impedences(rowID, 0).real();
        v(rowID * 8 + 1) = impedences(rowID, 0).imag();
        v(rowID * 8 + 2) = impedences(rowID, 1).real();
        v(rowID * 8 + 3) = impedences(rowID, 1).imag();
        v(rowID * 8 + 4) = impedences(rowID, 2).real();
        v(rowID * 8 + 5) = impedences(rowID, 2).imag();
        v(rowID * 8 + 6) = impedences(rowID, 3).real();
        v(rowID * 8 + 7) = impedences(rowID, 3).imag();
      }
      return v;
    }

    Eigen::VectorXd mtApparentResLikelihoodVector(const Eigen::MatrixX4cd& impedences, const Eigen::VectorXd& freqs)
    {
      Eigen::VectorXd v(impedences.rows()*2);
      CHECK_EQ(freqs.size(), impedences.rows());
      for (uint rowID = 0; rowID < impedences.rows(); rowID++)
      {
        v(2*rowID) = std::pow(std::abs(impedences(rowID, 1)), 2) * 0.2 / freqs[rowID];
        v(2*rowID+1) =  std::pow(std::abs(impedences(rowID, 2)), 2) * 0.2 / freqs[rowID];
      }
      return v;
    }

    template<>
    double likelihood<ForwardModel::MTANISO>(const MtAnisoResults& synthetic, const MtAnisoResults& real, const MtAnisoSpec& spec)
    {
      if (real.readings.size() == 0)
        return 0.0;
      double likelihood = 0;
      std::vector<Eigen::VectorXd> realReadings;
      std::vector<Eigen::VectorXd> synReadings;
      for (uint i = 0; i < real.readings.size(); i++)
      {
        realReadings.push_back(mtApparentResLikelihoodVector(real.readings[i], spec.freqs[i]));
        synReadings.push_back(mtApparentResLikelihoodVector(synthetic.readings[i], spec.freqs[i]));
      }
      double sigma = stdDev(realReadings);
      VLOG(3) << "MT likelihood sigma: " << sigma;
      for (uint i = 0; i < real.readings.size(); i++)
      {
        // 0.5 because we're essentially doubling the MT with the TE and TM
        // modes
        double t = 0.5 * lh::normalInverseGamma(realReadings[i] / sigma, synReadings[i] / sigma, spec.noise.inverseGammaAlpha,
                                          spec.noise.inverseGammaBeta);
        VLOG(3) << "MT Reading " << i << " likelihood:" << t;
        likelihood += t;
      }
      VLOG(2) << "MT Aniso Likelihood: " << likelihood;

      return likelihood;
    }

    template<>
    double likelihood<ForwardModel::SEISMIC1D>(const Seismic1dResults& synthetic, const Seismic1dResults& real, const Seismic1dSpec& spec)
    {
      if (real.readings.size() == 0)
        return 0.0;
      double l = 0;
      double sigma = stdDev(real.readings);
      VLOG(3) << "Seismic likelihood sigma: " << sigma;
      for (uint i = 0; i < synthetic.readings.size(); i++)
      {
        double t = normalInverseGamma(real.readings[i] / sigma, synthetic.readings[i] / sigma, spec.noise.inverseGammaAlpha,
                                      spec.noise.inverseGammaBeta);
        l += t;
        VLOG(3) << "Seismic Reading " << i << " likelihood:" << t;
      }
      VLOG(2) << "Seismic Likelihood: " << l;
      return l;
    }

    template<>
    double likelihood<ForwardModel::CONTACTPOINT>(const ContactPointResults& synthetic, const ContactPointResults& real,
                                                  const ContactPointSpec& spec)
    {
      if (real.readings.size() == 0)
        return 0.0;
      double l = 0;
      double sigma = stdDev(real.readings);
      VLOG(3) << "Contact Point likelihood sigma: " << sigma;
      for (uint i = 0; i < synthetic.readings.size(); i++)
      {
        double t = normalInverseGamma(real.readings[i] / sigma, synthetic.readings[i] / sigma, spec.noise.inverseGammaAlpha,
                                      spec.noise.inverseGammaBeta);
        l += t;
        VLOG(3) << "Contact Point Reading " << i << " likelihood:" << t;
      }
      VLOG(2) << "Contact point Likelihood: " << l;
      return l;
    }

    template<>
    double likelihood<ForwardModel::THERMAL>(const ThermalResults& synthetic, const ThermalResults& real, const ThermalSpec& spec)
    {
      if (real.readings.size() == 0)
        return 0.0;
      double sigma = stdDev(real.readings);
      VLOG(3) << "Thermal likelihood sigma: " << sigma;
      double l = lh::normalInverseGamma(synthetic.readings / sigma, real.readings / sigma, spec.noise.inverseGammaAlpha,
                                        spec.noise.inverseGammaBeta);
      VLOG(2) << "Thermal Likelihood: " << l;
      return l;
    }

    std::vector<double> likelihoodAll(const GlobalResults& synthetic, const GlobalResults& real, const GlobalSpec& spec,
                                      const std::set<ForwardModel>& enabled)
    {
      std::vector<double> lh;
      lh.push_back(enabled.count(ForwardModel::GRAVITY) ? likelihood<ForwardModel::GRAVITY>(synthetic.grav, real.grav, spec.grav) : 0.0);
      lh.push_back(enabled.count(ForwardModel::MAGNETICS) ? likelihood<ForwardModel::MAGNETICS>(synthetic.mag, real.mag, spec.mag) : 0.0);
      lh.push_back(enabled.count(ForwardModel::MTANISO) ? likelihood<ForwardModel::MTANISO>(synthetic.mt, real.mt, spec.mt) : 0.0);
      lh.push_back(enabled.count(ForwardModel::SEISMIC1D) ? likelihood<ForwardModel::SEISMIC1D>(synthetic.s1d, real.s1d, spec.s1d) : 0.0);
      lh.push_back(
          enabled.count(ForwardModel::CONTACTPOINT) ?
              likelihood<ForwardModel::CONTACTPOINT>(synthetic.cpoint, real.cpoint, spec.cpoint) : 0.0);
      lh.push_back(enabled.count(ForwardModel::THERMAL) ? likelihood<ForwardModel::THERMAL>(synthetic.therm, real.therm, spec.therm) : 0.0);
      return lh;
    }
  }
}
