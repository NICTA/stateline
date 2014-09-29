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
  return veigen2lnumpy(adapter.sigmas());
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

py::object sigmaCovarianceAdapterSigmas(const mcmc::SigmaCovarianceAdapter &adapter)
{
  return veigen2lnumpy(adapter.sigmas());
}

py::object sigmaCovarianceAdapterAcceptRates(const mcmc::SigmaCovarianceAdapter &adapter)
{
  return vector2list(adapter.acceptRates());
}

py::object sigmaCovarianceAdapterSampleCov(const mcmc::SigmaCovarianceAdapter &adapter, uint i)
{
  Eigen::MatrixXd cov = adapter.covs()[i];
  return eigen2numpy(Eigen::Map<Eigen::VectorXd>(cov.data(), cov.size()));
}

boost::shared_ptr<mcmc::BlockSigmaAdapter>
  blockSigmaAdapterInit(uint nStacks, uint nChains, uint nDims,
      const py::list &blocks, const mcmc::SlidingWindowSigmaSettings &settings)
{
  return boost::shared_ptr<mcmc::BlockSigmaAdapter>(
      new mcmc::BlockSigmaAdapter(nStacks, nChains, nDims,
        list2vector<uint>(blocks), settings));
}

py::object blockSigmaAdapterSigmas(const mcmc::BlockSigmaAdapter &adapter)
{
  return veigen2lnumpy(adapter.sigmas());
}

py::object blockSigmaAdapterAcceptRates(const mcmc::BlockSigmaAdapter &adapter)
{
  return vector2list(adapter.acceptRates());
}

void exportSlidingWindowSigmaAdapter()
{
  py::class_<mcmc::SlidingWindowSigmaAdapter, boost::noncopyable>("SlidingWindowSigmaAdapter",
      py::init<uint, uint, uint, mcmc::SlidingWindowSigmaSettings>())
    .def("update", &mcmc::SlidingWindowSigmaAdapter::update)
    .def("sigmas", slidingWindowSigmaAdapterSigmas)
    .def("accept_rates", slidingWindowSigmaAdapterAcceptRates)
  ;
}

void exportSlidingWindowBetaAdapter()
{
  py::class_<mcmc::SlidingWindowBetaAdapter, boost::noncopyable>("SlidingWindowBetaAdapter",
      py::init<uint, uint, mcmc::SlidingWindowBetaSettings>())
    .def("update", &mcmc::SlidingWindowBetaAdapter::update)
    .def("betas", slidingWindowBetaAdapterBetas)
    .def("swap_rates", slidingWindowBetaAdapterSwapRates)
  ;
}

void exportSigmaCovarianceAdapter()
{
  py::class_<mcmc::SigmaCovarianceAdapter, boost::noncopyable>("SigmaCovarianceAdapter",
      py::init<uint, uint, uint, mcmc::SlidingWindowSigmaSettings>())
    .def("update", &mcmc::SigmaCovarianceAdapter::update)
    .def("sigmas", sigmaCovarianceAdapterSigmas)
    .def("accept_rates", sigmaCovarianceAdapterAcceptRates)
    .def("sample_cov", sigmaCovarianceAdapterSampleCov)
  ;
}

void exportBlockSigmaAdapter()
{
  py::class_<mcmc::BlockSigmaAdapter, boost::noncopyable>("BlockSigmaAdapter", py::no_init)
    .def("__init__", py::make_constructor(blockSigmaAdapterInit))
    .def("update", &mcmc::BlockSigmaAdapter::update)
    .def("sigmas", blockSigmaAdapterSigmas)
    .def("accept_rates", blockSigmaAdapterAcceptRates)
  ;
}
