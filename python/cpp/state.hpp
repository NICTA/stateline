#pragma once

#include "infer/datatypes.hpp"

void stateSetSample(mcmc::State &self, py::object sample)
{
  self.sample = numpy2eigen(sample);
}

py::object stateGetSample(mcmc::State &self)
{
  return eigen2numpy(self.sample);
}

void stateSetSigma(mcmc::State &self, py::object sigma)
{
  self.sigma = numpy2eigen(sigma);
}

py::object stateGetSigma(mcmc::State &self)
{
  return eigen2numpy(self.sigma);
}

void exportState()
{
  py::class_<mcmc::State>("State")
    .def("get_sample", stateGetSample)
    .def("set_sample", stateSetSample)
    .def_readwrite("energy", &mcmc::State::energy)
    .def("get_sigma", stateGetSigma)
    .def("set_sigma", stateSetSigma)
    .def_readwrite("beta", &mcmc::State::beta)
    .def_readwrite("accepted", &mcmc::State::accepted)
    .def_readwrite("swap_type", &mcmc::State::swapType);
}
