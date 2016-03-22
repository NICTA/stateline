# Copyright (c) 2014, NICTA.
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014

ADD_LIBRARY(mcmc OBJECT sampler.cpp chainarray.cpp diagnostics.cpp adaptive.cpp logging.cpp)

#MESSAGE(STATUS "Linking mcmc to ${Boost_SERIALIZATION_LIBRARY}")
#TARGET_LINK_LIBRARIES(mcmc db ${Boost_SERIALIZATION_LIBRARY})

#ADD_TEST(test-diagnostics tests/diagnostics.cpp ${Boost_SYSTEM_LIBRARY} mcmc)
