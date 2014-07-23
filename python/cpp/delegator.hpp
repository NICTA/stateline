#pragma once

#include "comms/delegator.hpp"

void exportHeartbeatSettings()
{
  py::class_<HeartbeatSettings>("HeartbeatSettings")
    .def_readwrite("rate", &HeartbeatSettings::msRate)
    .def_readwrite("poll_rate", &HeartbeatSettings::msPollRate)
    .def_readwrite("timeout", &HeartbeatSettings::msTimeout);
}

void exportDelegatorSettings()
{
  py::class_<DelegatorSettings>("DelegatorSettings")
    .def_readwrite("poll_rate", &DelegatorSettings::msPollRate)
    .def_readwrite("port", &DelegatorSettings::port)
    .def_readwrite("heartbeat", &DelegatorSettings::heartbeat);
}

boost::shared_ptr<comms::Delegator>
  delegatorInit(const std::string &globalSpec,
      const py::dict &jobSpec, const DelegatorSettings &settings)
{
  return boost::shared_ptr<comms::Delegator>(
      new comms::Delegator(
        globalSpec,
        dict2map<comms::JobID, std::string>(jobSpec),
        settings));
}

void exportDelegator()
{
  exportHeartbeatSettings();
  exportDelegatorSettings();

  py::class_<comms::Delegator, boost::noncopyable>("Delegator", py::no_init)
    .def("__init__", py::make_constructor(&delegatorInit))
    .def("start", &comms::Delegator::start)
    .def("stop", &comms::Delegator::stop)
  ;
}
