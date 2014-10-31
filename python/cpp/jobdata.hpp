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

void setGlobalData(comms::JobData &self, const std::string &value)
{
  self.globalData = value;
}

void setJobData(comms::JobData &self, const std::string &value)
{
  self.jobData = value;
}

void exportJobData()
{
  py::class_<comms::JobData>("JobData",
      py::init<uint, const std::string &, const std::string &>())
    .def_readwrite("job_type", &comms::JobData::type)
    .def("get_global_data", getGlobalData)
    .def("get_job_data", getJobData)
    .def("set_global_data", setGlobalData)
    .def("set_job_data", setJobData)
  ;
}
