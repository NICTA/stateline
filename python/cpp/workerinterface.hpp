#pragma once

#include "infer/datatypes.hpp"

boost::shared_ptr<mcmc::WorkerInterface>
  workerInterfaceInit(const std::string &globalSpec, py::dict jobSpec, py::object jobConstructFn, py::object resultEnergyFn, uint port)
{
  return boost::shared_ptr<mcmc::WorkerInterface>(
      new mcmc::WorkerInterface(
        globalSpec,
        dict2map<uint, std::string>(jobSpec),
        [jobConstructFn](const Eigen::VectorXd &x)
        {
          std::cout << "CONSTRUCTING: " << x << std::endl;
          py::object jobs = jobConstructFn(eigen2numpy(x));
          // TODO: verify jobs is a list
          return list2vector<comms::JobData>((py::list)jobs);
        },
        [resultEnergyFn](const std::vector<comms::ResultData> &x)
        {
          return py::extract<double>(resultEnergyFn(vector2list(x)))();
        },
        DelegatorSettings::Default(port)
      ));
}

void workerInterfaceSubmit(mcmc::WorkerInterface &self, uint id, py::object x)
{
  self.submit(id, numpy2eigen(x));
}

py::tuple workerInterfaceRetrieve(mcmc::WorkerInterface &self)
{
  std::pair<uint, double> result = self.retrieve();
  return py::make_tuple(result.first, result.second);
}

void exportWorkerInterface()
{
  py::class_<mcmc::WorkerInterface, boost::noncopyable>("WorkerInterface", py::no_init)
    .def("__init__", py::make_constructor(workerInterfaceInit))
    .def("submit", workerInterfaceSubmit)
    .def("retrieve", workerInterfaceRetrieve)
  ;
}
