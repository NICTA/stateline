#include "infer/chainarray.hpp"

void exportChainSettings()
{
  py::class_<mcmc::ChainSettings>("ChainSettings")
    .def_readwrite("recover_from_disk", &mcmc::ChainSettings::recoverFromDisk)
    .def_readwrite("database_path", &mcmc::ChainSettings::databasePath)
    .def_readwrite("chain_cache_length", &mcmc::ChainSettings::chainCacheLength)
    .def_readwrite("database_cache_size_MB", &mcmc::ChainSettings::databaseCacheSizeMB)
  ;
}

py::object chainGetStates(mcmc::ChainArray &self, uint id, uint nburn, uint nthin)
{
  return vector2list(self.states(id, nburn, nthin));
}

void chainInitialise(mcmc::ChainArray &self, uint id,
    py::object sample, double energy, double sigma, double beta)
{
  self.initialise(id, numpy2eigen(sample), energy, sigma, beta);
}

bool chainAppend(mcmc::ChainArray &self, uint id, py::object sample, double energy)
{
  return self.append(id, numpy2eigen(sample), energy);
}

void exportChainArray()
{
  py::class_<mcmc::ChainArray, boost::noncopyable>("ChainArray",
      py::init<uint, uint, const mcmc::ChainSettings &>())
    .def("length", &mcmc::ChainArray::length)
    .def("sigma", &mcmc::ChainArray::sigma)
    .def("set_sigma", &mcmc::ChainArray::setSigma)
    .def("beta", &mcmc::ChainArray::beta)
    .def("set_beta", &mcmc::ChainArray::setBeta)
    .def("initialise", chainInitialise)
    .def("last_state", &mcmc::ChainArray::lastState)
    .def("append", chainAppend)
    .def("state", &mcmc::ChainArray::state)
    .def("states", chainGetStates)
    .def("swap", &mcmc::ChainArray::swap)
    .def("stack_index", &mcmc::ChainArray::stackIndex)
    .def("chain_index", &mcmc::ChainArray::chainIndex)
    .def("is_hottest", &mcmc::ChainArray::isHottestInStack)
    .def("is_coldest", &mcmc::ChainArray::isColdestInStack)
  ;
}
