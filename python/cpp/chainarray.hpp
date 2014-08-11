#pragma once

#include "infer/chainarray.hpp"

void exportDBSettings()
{
  py::class_<DBSettings>("DBSettings")
    .def_readwrite("directory", &DBSettings::directory)
    .def_readwrite("recover", &DBSettings::recover)
    .def_readwrite("cacheSizeMB", &DBSettings::cacheSizeMB);
}

void exportChainArray()
{
  exportDBSettings();

  py::class_<mcmc::ChainArray, boost::noncopyable>("ChainArray",
      py::init<uint, uint, double ,double, double, const DBSettings&,uint>())
    .def("length", &comms::Delegator::start)
  ;
}
