#pragma once

#include "comms/datatypes.hpp"

py::object getResultData(comms::ResultData &self)
{
  return string2bytes(self.data);
}

void exportResultData()
{
  py::class_<comms::ResultData>("ResultData")
    .def_readwrite("type", &comms::ResultData::type)
    .def("get_data", getResultData)
    .def_readwrite("data", &comms::ResultData::data)
  ;
}
