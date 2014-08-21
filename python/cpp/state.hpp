#pragma once

#include "infer/state.hpp"

py::object getStateSample(const mcmc::State &self)
{
    return eigen2numpy(self.sample);
}

py::object getStateSigma(const mcmc::State &self)
{
    return eigen2numpy(self.sigma);
}

void exportState()
{
  py::class_<mcmc::State>("State")
    .def("get_sample", &getStateSample)
    .def_readwrite("energy", &mcmc::State::energy)
    .def("get_sigma", &getStateSigma)
    .def_readwrite("beta", &mcmc::State::beta)
    .def_readwrite("accepted", &mcmc::State::accepted);

  // TODO: swap type (which is an enum, don't know the best way to do it)
}
