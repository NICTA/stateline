#pragma once

#include "comms/settings.hpp"

void exportHeartbeatSettings()
{
  py::class_<HeartbeatSettings>("HeartbeatSettings")
    .def_readwrite("rate", &HeartbeatSettings::msRate)
    .def_readwrite("poll_rate", &HeartbeatSettings::msPollRate)
    .def_readwrite("timeout", &HeartbeatSettings::msTimeout)
    ;
}

void exportDelegatorSettings()
{
  py::class_<DelegatorSettings>("DelegatorSettings")
    .def_readwrite("poll_rate", &DelegatorSettings::msPollRate)
    .def_readwrite("port", &DelegatorSettings::port)
    .def_readwrite("heartbeat", &DelegatorSettings::heartbeat);
}

void exportWorkerSettings()
{
  py::class_<WorkerSettings>("WorkerSettings")
    .def_readwrite("poll_rate", &WorkerSettings::msPollRate)
    .def_readwrite("address", &WorkerSettings::address)
    .def_readwrite("heartbeat", &WorkerSettings::heartbeat)
    ;
}

void exportSettings()
{
  exportHeartbeatSettings();
  exportDelegatorSettings();
  exportWorkerSettings();
}
