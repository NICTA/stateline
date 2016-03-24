# Copyright (c) 2016, NICTA.
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Hamish Ivey-Law
# Date: 2016


if (NOT GTEST_ROOT)
  SET(GTEST_ROOT "/usr/src/gtest")
endif()

find_path(GTEST_SRC_DIR src/gtest-all.cc
    HINTS $ENV{GTEST_ROOT} ${GTEST_ROOT})
find_path(GTEST_INCLUDE_DIR gtest/gtest.h
    HINTS $ENV{GTEST_ROOT}/include ${GTEST_ROOT}/include)
mark_as_advanced(GTEST_INCLUDE_DIR GTEST_SRC_DIR)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(GTest
  "Couldn't find GTest; please check your GTEST_ROOT setting!"
  GTEST_INCLUDE_DIR GTEST_SRC_DIR)

INCLUDE_DIRECTORIES(SYSTEM "${GTEST_SRC_DIR}"
                    SYSTEM "${GTEST_INCLUDE_DIR}")
ADD_LIBRARY(gtest EXCLUDE_FROM_ALL ${GTEST_SRC_DIR}/src/gtest-all.cc)

SET(test_suite_name test-suite)

ADD_CUSTOM_TARGET(check
  COMMAND ${test_suite_name}
  DEPENDS ${test_suite_name}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Run unit tests")

ADD_EXECUTABLE(${test_suite_name} EXCLUDE_FROM_ALL
  main.cpp
  delegator.cpp diagnostics.cpp message.cpp router.cpp socket.cpp
  # chainarray.cpp db.cpp transport.cpp
  )
TARGET_LINK_LIBRARIES(${test_suite_name} statelineserver statelineclient gtest)
