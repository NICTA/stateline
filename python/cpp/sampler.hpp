#pragma once

#include "infer/sampler.hpp"

boost::shared_ptr<mcmc::Sampler>
  samplerInit(mcmc::WorkerInterface &interface,
      mcmc::ChainArray &chain, py::object propFn, uint swapInterval)
{
  return boost::shared_ptr<mcmc::Sampler>(
      new mcmc::Sampler(
        interface,
        chain,
        [propFn](uint id, const Eigen::VectorXd &sample, const Eigen::VectorXd &sigma)
        {
          return numpy2eigen(propFn(id, eigen2numpy(sample), eigen2numpy(sigma)));
        },
        swapInterval
      )
    );
}

py::tuple samplerStep(mcmc::Sampler &sampler, py::list sigmas, py::list betas)
{
  auto result = sampler.step(lnumpy2veigen(sigmas), list2vector<double>(betas));
  return py::make_tuple(result.first, result.second);
}

void exportSampler()
{
  py::class_<mcmc::Sampler, boost::noncopyable>("Sampler", py::no_init)
    .def("__init__", py::make_constructor(samplerInit))
    .def("step", &samplerStep)
    .def("flush", &mcmc::Sampler::flush)
  ;
}
