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

void exportSwapType()
{
  py::enum_<mcmc::SwapType>("SwapType")
    .value("NO_ATTEMPT", mcmc::SwapType::NoAttempt)
    .value("ACCEPT", mcmc::SwapType::Accept)
    .value("REJECT", mcmc::SwapType::Reject)
  ;
}

void exportState()
{
  py::class_<mcmc::State>("State")
    .def("get_sample", stateGetSample)
    .def("set_sample", stateSetSample)
    .def_readwrite("energy", &mcmc::State::energy)
    .def_readwrite("sigma", &mcmc::State::sigma)
    .def_readwrite("beta", &mcmc::State::beta)
    .def_readwrite("accepted", &mcmc::State::accepted)
    .def_readwrite("swap_type", &mcmc::State::swapType)
  ;
}
