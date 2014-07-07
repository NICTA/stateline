# Copyright (c) 2014, NICTA. 
# Affero General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

MESSAGE ( STATUS "GTEST directory: ${GTEST_SRC_DIR}" )
SET(GTEST_DIR ${GTEST_SRC_DIR})
SET ( GTEST_INCLUDE_DIR ${GTEST_SRC_DIR}/include )

INCLUDE_DIRECTORIES (${GTEST_DIR}
                     ${GTEST_INCLUDE_DIR}
                     ${EIGEN3_INCLUDE_DIR})
ADD_LIBRARY( gtest 
             ${GTEST_DIR}/src/gtest-all.cc)

TARGET_LINK_LIBRARIES(gtest pthread)

ADD_LIBRARY( test-common common.cpp)

TARGET_LINK_LIBRARIES(test-common gtest)
