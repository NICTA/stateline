#pragma once

#include "comms/datatypes.hpp"

py::object getResultData(comms::ResultData &self)
{
  return string2bytes(self.data);
}

void setResultData(comms::ResultData &self, const std::string &value)
{
  self.data = value;
}

void exportResultData()
{
  py::class_<comms::ResultData>("ResultData",
      py::init<uint, const std::string &>())
    .def_readwrite("job_type", &comms::ResultData::type)
    .def("get_data", getResultData)
    .def("set_data", setResultData)
  ;
}
