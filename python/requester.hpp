#pragma once

#include "comms/requester.hpp"

void exportJobData()
{
  py::class_<comms::JobData>("JobData")
    .def_readwrite("type", &comms::JobData::type)
    .def_readwrite("global_data", &comms::JobData::globalData)
    .def_readwrite("job_data", &comms::JobData::jobData)
  ;
}

void exportResultData()
{
  py::class_<comms::ResultData>("ResultData")
    .def_readwrite("type", &comms::ResultData::type)
    .def_readwrite("data", &comms::ResultData::data)
  ;
}

py::tuple requesterRetrieve(comms::Requester &self)
{
  std::pair<uint, comms::ResultData> result = self.retrieve();
  return py::make_tuple(result.first, result.second);
}

void exportRequester()
{
  exportJobData();
  exportResultData();

  py::class_<comms::Requester, boost::noncopyable>("Requester",
      py::init<comms::Delegator &>())
    .def("submit", &comms::Requester::submit)
    .def("retrieve", &requesterRetrieve)
  ;
}
