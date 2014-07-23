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

void requesterBatchSubmit(comms::Requester &self, uint id, const py::list &jobs)
{
  self.batchSubmit(id, list2vector<comms::JobData>(jobs));
}

py::tuple requesterBatchRetrieve(comms::Requester &self)
{
  std::pair<uint, std::vector<comms::ResultData>> result = self.batchRetrieve();
  return py::make_tuple(result.first, vector2list(result.second));
}

void exportRequester()
{
  exportJobData();
  exportResultData();

  py::class_<comms::Requester, boost::noncopyable>("Requester",
      py::init<comms::Delegator &>())
    .def("batch_submit", &requesterBatchSubmit)
    .def("batch_retrieve", &requesterBatchRetrieve)
  ;
}
