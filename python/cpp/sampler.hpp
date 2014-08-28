#pragma once

#include "infer/sampler.hpp"

void samplerStep(mcmc::Sampler &sampler, py::list sigmas, py::list betas)
{
  sampler.step(lnumpy2veigen(sigmas), list2vector<double>(betas));
}

void exportSampler()
{
  py::class_<mcmc::Sampler, boost::noncopyable>("Sampler",
      .def("step", &samplerStep)
      .def("flush", &mcmc::Sampler::flush)
  ;
}
