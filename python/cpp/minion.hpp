#pragma once

#include "comms/minion.hpp"

void exportMinion()
{
  py::class_<comms::Minion, boost::noncopyable>("Minion",
      py::init<comms::Worker &, uint>())
    .def("next_job", &comms::Minion::nextJob)
    .def("submit_result", &comms::Minion::submitResult)
  ;
}
