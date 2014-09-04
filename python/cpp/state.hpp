#pragma once

#include "infer/datatypes.hpp"

void setStateSample(mcmc::State &self, py::object sample)
{
  self.sample = numpy2eigen(sample);
}

void setStateSigma(mcmc::State &self, py::object sigma)
{
  self.sigma = numpy2eigen(sigma);
}

void exportState()
{
  py::class_<mcmc::State>("State")
    .def("set_sample", setStateSample)
    .def_readwrite("energy", &mcmc::State::energy)
    .def("set_sigma", setStateSigma)
    .def_readwrite("beta", &mcmc::State::beta)
    .def_readwrite("accepted", &mcmc::State::accepted)
    .def_readwrite("swap_type", &mcmc::State::swapType);
}
