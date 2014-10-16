#pragma once

#include "infer/adaptive.hpp"

void exportSlidingWindowSigmaSettings()
{
  py::class_<mcmc::SlidingWindowSigmaSettings>("SlidingWindowSigmaSettings")
    .def_readwrite("window_size", &mcmc::SlidingWindowSigmaSettings::windowSize)
    .def_readwrite("cold_sigma", &mcmc::SlidingWindowSigmaSettings::coldSigma)
    .def_readwrite("sigma_factor", &mcmc::SlidingWindowSigmaSettings::sigmaFactor)
    .def_readwrite("adaption_length", &mcmc::SlidingWindowSigmaSettings::adaptionLength)
    .def_readwrite("nsteps_per_adapt", &mcmc::SlidingWindowSigmaSettings::nStepsPerAdapt)
    .def_readwrite("optimal_accept_rate", &mcmc::SlidingWindowSigmaSettings::optimalAcceptRate)
    .def_readwrite("adapt_rate", &mcmc::SlidingWindowSigmaSettings::adaptRate)
    .def_readwrite("min_adapt_factor", &mcmc::SlidingWindowSigmaSettings::minAdaptFactor)
    .def_readwrite("max_adapt_factor", &mcmc::SlidingWindowSigmaSettings::maxAdaptFactor)
  ;
}

void exportSlidingWindowBetaSettings()
{
  py::class_<mcmc::SlidingWindowBetaSettings>("SlidingWindowBetaSettings")
    .def_readwrite("window_size", &mcmc::SlidingWindowBetaSettings::windowSize)
    .def_readwrite("beta_factor", &mcmc::SlidingWindowBetaSettings::betaFactor)
    .def_readwrite("adaption_length", &mcmc::SlidingWindowBetaSettings::adaptionLength)
    .def_readwrite("nsteps_per_adapt", &mcmc::SlidingWindowBetaSettings::nStepsPerAdapt)
    .def_readwrite("optimal_swap_rate", &mcmc::SlidingWindowBetaSettings::optimalSwapRate)
    .def_readwrite("adapt_rate", &mcmc::SlidingWindowBetaSettings::adaptRate)
    .def_readwrite("min_adapt_factor", &mcmc::SlidingWindowBetaSettings::minAdaptFactor)
    .def_readwrite("max_adapt_factor", &mcmc::SlidingWindowBetaSettings::maxAdaptFactor)
  ;
}

py::object slidingWindowSigmaAdapterSigmas(const mcmc::SlidingWindowSigmaAdapter &adapter)
{
  return vector2list(adapter.sigmas());
}

py::object slidingWindowSigmaAdapterAcceptRates(const mcmc::SlidingWindowSigmaAdapter &adapter)
{
  return vector2list(adapter.acceptRates());
}

py::object slidingWindowBetaAdapterBetas(const mcmc::SlidingWindowBetaAdapter &adapter)
{
  return vector2list(adapter.betas());
}

py::object slidingWindowBetaAdapterSwapRates(const mcmc::SlidingWindowBetaAdapter &adapter)
{
  return vector2list(adapter.swapRates());
}

void covarianceEstimatorUpdate(mcmc::CovarianceEstimator &self,
    uint id, py::object sample)
{
  self.update(id, numpy2eigen(sample));
}

py::object covarianceEstimatorCov(mcmc::CovarianceEstimator &self,
    uint id)
{
  return eigen2d2numpy(self.covariances()[id]);
}

void exportSlidingWindowSigmaAdapter()
{
  py::class_<mcmc::SlidingWindowSigmaAdapter>("SlidingWindowSigmaAdapter",
      py::init<uint, uint, uint, mcmc::SlidingWindowSigmaSettings>())
    .def("update", &mcmc::SlidingWindowSigmaAdapter::update)
    .def("sigmas", slidingWindowSigmaAdapterSigmas)
    .def("accept_rates", slidingWindowSigmaAdapterAcceptRates)
  ;
}

void exportSlidingWindowBetaAdapter()
{
  py::class_<mcmc::SlidingWindowBetaAdapter>("SlidingWindowBetaAdapter",
      py::init<uint, uint, mcmc::SlidingWindowBetaSettings>())
    .def("update", &mcmc::SlidingWindowBetaAdapter::update)
    .def("betas", slidingWindowBetaAdapterBetas)
    .def("swap_rates", slidingWindowBetaAdapterSwapRates)
  ;
}

void exportCovarianceEstimator()
{
  py::class_<mcmc::CovarianceEstimator>("CovarianceEstimator",
      py::init<uint, uint, uint>())
    .def("update", covarianceEstimatorUpdate)
    .def("cov", covarianceEstimatorCov)
  ;
}
