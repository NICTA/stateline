#pragma once

#include "infer/sampler.hpp"

py::tuple samplerStep(mcmc::Sampler &sampler, py::list sigmas, py::list betas)
{
  auto result = sampler.step(lnumpy2veigen(sigmas), list2vector<double>(betas));
  return py::make_tuple(result.first, result.second);
}

void exportSampler()
{
  py::class_<mcmc::Sampler, boost::noncopyable>("Sampler",
      .def("step", &samplerStep)
      .def("flush", &mcmc::Sampler::flush)
  ;
}
