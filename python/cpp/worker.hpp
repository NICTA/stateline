#pragma once

#include "comms/worker.hpp"

void exportWorkerSettings()
{
  py::class_<WorkerSettings>("WorkerSettings")
    .def_readwrite("poll_rate", &WorkerSettings::msPollRate)
    .def_readwrite("address", &WorkerSettings::address)
    .def_readwrite("heartbeat", &WorkerSettings::heartbeat)
  ;
}

boost::shared_ptr<comms::Worker>
  workerInit(const py::list &jobIDs, const WorkerSettings &settings)
{
  return boost::shared_ptr<comms::Worker>(
      new comms::Worker(list2vector<uint>(jobIDs),
        settings));
}

py::object workerGlobalSpec(comms::Worker &worker)
{
  return py::object(py::handle<>(PyUnicode_FromString(worker.globalSpec().c_str())));
  //std::wstringstream ws;
  //ws << worker.globalSpec().c_str();
  //return ws.str();
  //return worker.globalSpec();
}

std::string workerJobSpec(comms::Worker &worker, uint jobID)
{
  return worker.jobSpec(jobID);
}

void exportWorker()
{
  exportWorkerSettings();

  py::class_<comms::Worker, boost::noncopyable>("Worker", py::no_init)
    .def("__init__", py::make_constructor(&workerInit))
    .def("stop", &comms::Worker::stop)
    .def("global_spec", &workerGlobalSpec)
    .def("job_spec", &workerJobSpec)
  ;
}
