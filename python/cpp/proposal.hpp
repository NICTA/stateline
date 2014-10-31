#pragma once

#include "infer/sampler.hpp"

py::object gaussianProposal(uint id, py::object sample, double sigma)
{
  return eigen2numpy(mcmc::gaussianProposal(id, numpy2eigen(sample), sigma));
}

void gaussianCovProposalUpdate(mcmc::GaussianCovProposal &self,
    uint id, py::object cov)
{
  return self.update(id, numpy2eigen2d(cov));
}

py::object gaussianCovProposalPropose(mcmc::GaussianCovProposal &self,
    uint id, py::object sample, double sigma)
{
  return eigen2numpy(self.propose(id, numpy2eigen(sample), sigma));
}

void exportGaussianProposal()
{
  py::def("gaussian_proposal", gaussianProposal);
}

void exportGaussianCovProposal()
{
  py::class_<mcmc::GaussianCovProposal>("GaussianCovProposal",
      py::init<uint, uint, uint>())
    .def("update", gaussianCovProposalUpdate)
    .def("propose", gaussianCovProposalPropose);
}
