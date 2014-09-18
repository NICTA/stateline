#pragma once

#include "infer/sampler.hpp"

py::object gaussianProposal(uint id, py::object sample, py::object sigma)
{
  return eigen2numpy(mcmc::gaussianProposal(id, numpy2eigen(sample), numpy2eigen(sigma)));
}

void exportGaussianProposal()
{
  py::def("gaussian_proposal", gaussianProposal);
}
