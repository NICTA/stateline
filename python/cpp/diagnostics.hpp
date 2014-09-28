#pragma once

#include "infer/diagnostics.hpp"

py::object EPSRRHat(mcmc::EPSRDiagnostic &self)
{
  return eigen2numpy(self.rHat());
}

void exportEPSRDiagnostic()
{
  py::class_<mcmc::EPSRDiagnostic>("EPSRDiagnostic",
      py::init<uint, uint, uint, double>())
    .def("update", &mcmc::EPSRDiagnostic::update)
    .def("r_hat", EPSRRHat)
    .def("has_converged", &mcmc::EPSRDiagnostic::hasConverged)
  ;
}
