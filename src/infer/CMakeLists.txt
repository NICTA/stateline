# Copyright (c) 2014, NICTA. 
# Affero General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

ADD_LIBRARY(chainarray chainarray.cpp metropolis.cpp)
TARGET_LINK_LIBRARIES(chainarray db ${Boost_SERIALIZATION_LIBRARY})

ADD_TEST(test-chainarray tests/chainarray.cpp chainarray
                         ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})

ADD_TEST(test-diagnostics tests/diagnostics.cpp ${Boost_SYSTEM_LIBRARY})
