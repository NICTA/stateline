# Copyright (c) 2016, NICTA.
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Hamish Ivey-Law
# Date: 2016

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
