# Copyright (c) 2015, NICTA.
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2015

ADD_LIBRARY(mcmc_sampler OBJECT sampler.cpp)
ADD_LIBRARY(mcmc_samplesarray OBJECT samplesarray.cpp)
ADD_LIBRARY(mcmc_chainarray OBJECT chainarray.cpp)
ADD_LIBRARY(mcmc_diagnostics OBJECT diagnostics.cpp)
ADD_LIBRARY(mcmc_adaptive OBJECT adaptive.cpp)
ADD_LIBRARY(mcmc_logging OBJECT logging.cpp)

ADD_LIBRARY(mcmc
  $<TARGET_OBJECTS:mcmc_sampler>
  $<TARGET_OBJECTS:mcmc_samplesarray>
  $<TARGET_OBJECTS:mcmc_chainarray>
  $<TARGET_OBJECTS:mcmc_diagnostics>
  $<TARGET_OBJECTS:mcmc_adaptive>
  $<TARGET_OBJECTS:mcmc_logging>)

ADD_TEST(test-diagnostics tests/diagnostics.cpp mcmc)
