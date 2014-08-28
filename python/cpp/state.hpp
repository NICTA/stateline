#pragma once

#include "infer/state.hpp"

py::object getStateSample(mcmc::State &self)
{
  return eigen2numpy(self.sample);
}

py::object getStateSigma(mcmc::State &self)
{
  return eigen2numpy(self.sigma);
}

void setStateSample(const mcmc::State &self, py::object sample)
{
  self.state = numpy2vector(sample);
}

void setStateSigma(const mcmc::State &self, py::object sigma)
{
  self.sigma = numpy2vector(sigma);
}

void exportState()
{
  py::class_<mcmc::State>("State")
    .def("get_sample", &getStateSample)
    .def("set_sample", &setStateSample)
    .def_readwrite("energy", &mcmc::State::energy)
    .def("get_sigma", &getStateSigma)
    .def("set_sigma", &setStateSigma)
    .def_readwrite("beta", &mcmc::State::beta)
    .def_readwrite("accepted", &mcmc::State::accepted)
    .def_readwrite("swap_type", &mcmc::State::swapType);
}
