#pragma once

#include "comms/datatypes.hpp"

py::object getGlobalData(comms::JobData &self)
{
  return string2bytes(self.globalData);
}

py::object getJobData(comms::JobData &self)
{
  return string2bytes(self.jobData);
}

void exportJobData()
{
  py::class_<comms::JobData>("JobData")
    .def_readwrite("type", &comms::JobData::type)
    .def("get_global_data", getGlobalData)
    .def("get_job_data", getJobData)
    .def_readwrite("global_data", &comms::JobData::globalData)
    .def_readwrite("job_data", &comms::JobData::jobData)
  ;
}
