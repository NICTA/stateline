#pragma once

#include "comms/requester.hpp"

py::object getGlobalData(comms::JobData &self)
{
  return string2bytes(self.globalData);
}

py::object getJobData(comms::JobData &self)
{
  return string2bytes(self.jobData);
}

py::object getResultData(comms::ResultData &self)
{
  return string2bytes(self.data);
}

void exportJobData()
{
  py::class_<comms::JobData>("JobData")
    .def_readwrite("type", &comms::JobData::type)
    .def("global_data", &getGlobalData)
    .def("job_data", &getJobData)
  ;
}

void exportResultData()
{
  py::class_<comms::ResultData>("ResultData")
    .def_readwrite("type", &comms::ResultData::type)
    .def("data", &getResultData)
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
