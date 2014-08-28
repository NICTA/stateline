#pragma once

#include "infer/settings.hpp"
#include "db/settings.hpp"
#include "comms/settings.hpp"

void exportMCMCSettings()
{
  py::class_<MCMCSettings>("MCMCSettings")
    .def_readwrite("nstacks", &MCMCSettings::stacks)
    .def_readwrite("nchains", &MCMCSettings::chains)
    .def_readwrite("swap_interval", &MCMCSettings::swapInterval)
    .def_readwrite("cache_length", &MCMCSettings::cacheLength)
    ;
}

void exportDBSettings()
{
  py::class_<DBSettings>("DBSettings")
    .def_readwrite("directory", &DBSettings::directory)
    .def_readwrite("cache_size", &DBSettings::cacheSizeMB)
    ;
}

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
  exportMCMCSettings();
  exportDBSettings();
  exportHeartbeatSettings();
  exportDelegatorSettings();
  exportWorkerSettings();
}
