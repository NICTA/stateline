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
  return string2bytes(worker.globalSpec());
}

py::object workerJobSpec(comms::Worker &worker, uint jobID)
{
  return string2bytes(worker.jobSpec(jobID));
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
