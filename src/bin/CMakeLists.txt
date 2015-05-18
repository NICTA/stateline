# Copyright (c) 2014, NICTA. 
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

# Macro for adding CPP binaries
FUNCTION(ADD_BINARY name library)
  ADD_EXECUTABLE (${name} ${name}.cpp)

  SET_TARGET_PROPERTIES(${name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${STATELINE_BINARY_DIR})

  TARGET_LINK_LIBRARIES(${name}
    ${library}
    stats
    ${GLOG_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY})
  INSTALL (TARGETS ${name} RUNTIME DESTINATION bin)
ENDFUNCTION()

ADD_BINARY(stateline-server statelineserver)
ADD_BINARY(stateline-client statelineclient)
ADD_BINARY(stateline statelineserver)
ADD_BINARY(demo-worker statelineclient)

# TODO: a bit fiddly
ADD_CUSTOM_COMMAND(TARGET stateline
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/config.json ${STATELINE_BINARY_DIR}/config.json)
