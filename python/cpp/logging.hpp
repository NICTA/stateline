#pragma once

#include "infer/logging.hpp"

void tableLoggerUpdate(mcmc::TableLogger &self, uint id, const mcmc::State &state,
    py::list sigmas, py::list acceptRates, py::list betas, py::list swapRates)
{
  self.update(id, state,
      list2vector<double>(sigmas), list2vector<double>(acceptRates),
      list2vector<double>(betas), list2vector<double>(swapRates));
}

void exportTableLogger()
{
  py::class_<mcmc::TableLogger>("TableLogger", py::init<uint, uint, uint>())
    .def("update", tableLoggerUpdate)
  ;
}
