# Copyright (c) 2014, NICTA. 
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

# Macro for adding CPP demos
FUNCTION(ADD_DEMO demoName library)
  ADD_EXECUTABLE (${demoName} ${demoName}.cpp)

  SET_TARGET_PROPERTIES(${demoName} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${STATELINE_BINARY_DIR})

  TARGET_LINK_LIBRARIES(${demoName}
    ${library}
    stats
    ${GLOG_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY})
  INSTALL (TARGETS ${demoName} RUNTIME DESTINATION bin)
ENDFUNCTION()

ADD_DEMO(demoDelegator statelineserver)
ADD_DEMO(headlessDelegator statelineserver)
ADD_DEMO(demoWorker statelineclient)
ADD_DEMO(headlessWorker statelineclient)
