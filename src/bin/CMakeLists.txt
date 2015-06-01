# Copyright (c) 2014, NICTA. 
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

# Macro for adding CPP binaries
#FUNCTION(ADD_BINARY name library)
#  ADD_EXECUTABLE (${name} ${name}.cpp)
#
#  SET_TARGET_PROPERTIES(${name} PROPERTIES
#    RUNTIME_OUTPUT_DIRECTORY ${STATELINE_BINARY_DIR})
#
#  TARGET_LINK_LIBRARIES(${name}
#    ${library}
#    stats
#    ${GLOG_LIBRARY}
#    ${Boost_SYSTEM_LIBRARY}
#    ${Boost_PROGRAM_OPTIONS_LIBRARY})
#  INSTALL (TARGETS ${name} RUNTIME DESTINATION bin)
#ENDFUNCTION()
#
#FUNCTION(COPY_FILE name)
#  IF (NOT EXISTS "${STATELINE_BINARY_DIR}/${name}")
#    CONFIGURE_FILE(${name} "${STATELINE_BINARY_DIR}/${name}" COPYONLY)
#  ENDIF()
#ENDFUNCTION()
#
#ADD_BINARY(stateline-server statelineserver)
#ADD_BINARY(stateline-client statelineclient)
#ADD_BINARY(stateline statelineserver)
#ADD_BINARY(demo-worker statelineclient)
#
## Copy over demo files
#COPY_FILE(cpp-demo-config.json)
#COPY_FILE(python-demo-config.json)
#COPY_FILE(demo-worker.py)
#COPY_FILE(vis.py)
#
## Create demo output directories
#FILE(MAKE_DIRECTORY ${STATELINE_BINARY_DIR}/cpp-demo-output)
#FILE(MAKE_DIRECTORY ${STATELINE_BINARY_DIR}/python-demo-output)
