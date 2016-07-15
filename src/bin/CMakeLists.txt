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

  TARGET_LINK_LIBRARIES(${name} ${library})
  INSTALL (TARGETS ${name} RUNTIME DESTINATION bin)
ENDFUNCTION()

FUNCTION(COPY_FILE name)
  IF (NOT EXISTS "${STATELINE_BINARY_DIR}/${name}")
    CONFIGURE_FILE(${name} "${STATELINE_BINARY_DIR}/${name}" COPYONLY)
  ENDIF()
ENDFUNCTION()

# no point building this until we have a need for it
#ADD_BINARY(stateline-server statelineserver)

#ADD_BINARY(stateline-client statelineclient)
#ADD_BINARY(stateline statelineserver)
ADD_BINARY(agent statelineclient)
