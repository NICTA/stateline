# Copyright (c) 2014, NICTA. 
# Affero General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

ADD_LIBRARY(chainarray chainarray.cpp
                       metropolis.cpp)

TARGET_LINK_LIBRARIES(chainarray ${Boost_SERIALIZATION_LIBRARY})
                     
#ADD_EXECUTABLE(test-chainarray testchainarray.cpp)
#TARGET_LINK_LIBRARIES(test-chainarray chainarray db serial ${obsidianBaseLibraries})
#REGISTER_UNIT_TESTS(test-chainarray)
#
#ADD_EXECUTABLE(test-diagnostics testdiagnostics.cpp)
#TARGET_LINK_LIBRARIES(test-diagnostics ${obsidianBaseLibraries})
#REGISTER_UNIT_TESTS(test-diagnostics)
