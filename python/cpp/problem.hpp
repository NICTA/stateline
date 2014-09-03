#pragma once

#include "infer/mcmc.hpp"

py::object getGlobalSpecData(mcmc::ProblemInstance &self)
{
  return string2bytes(self.globalJobSpecData);
}

py::object getJobSpecData(mcmc::ProblemInstance &self)
{
  return map2dict(self.perJobSpecData);
}

void setJobConstructFn(mcmc::ProblemInstance &self, py::object callable)
{
    self.jobConstructFn = mcmc::JobConstructFunction([callable](const Eigen::VectorXd &x)
    {
        return list2vector<comms::JobData>((py::list)callable(eigen2numpy(x)));
    });
}

void setResultEnergyFn(mcmc::ProblemInstance &self, py::object callable)
{
    self.resultEnergyFn = mcmc::ResultEnergyFunction([callable](const std::vector<comms::ResultData> &x)
    {
        return py::extract<double>(callable(vector2list(x)))();
    });
}

void exportProblemInstance()
{
  py::class_<mcmc::ProblemInstance>("ProblemInstance")
    .def("get_global_spec_data", getGlobalSpecData)
    .def("get_job_spec_data", &getJobSpecData)
    .def("set_job_construct_fn", &setJobConstructFn)
    .def("set_result_energy_fn", &setResultEnergyFn);
}