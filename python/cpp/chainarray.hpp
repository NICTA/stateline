#include "infer/chainarray.hpp"

void exportChainSettings()
{
  py::class_<mcmc::ChainSettings>("ChainSettings")
    .def_readwrite("recover_from_disk", mcmc::ChainSettings::recoverFromDisk)
    .def_readwrite("database_path", mcmc::ChainSettings::databasePath)
    .def_readwrite("chain_cache_length", mcmc::ChainSettings::chainCacheLength)
    .def_readwrite("database_cache_size_MB", mcmc::ChainSettings::databaseCacheSizeMB)
  ;
}

void exportChainArray()
{
  py::class_<mcmc::ChainArray, boost::noncopyable>("ChainArray",
      py::init<uint, uint, const comms::ChainSettings &>())
    .def("length", mcmc::ChainArray::length)
  ;
}

