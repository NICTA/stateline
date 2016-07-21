# Copyright (c) 2014, NICTA. 
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

# Macro for adding CPP binaries
FUNCTION(ADD_BINARY fname name library)
  ADD_EXECUTABLE(${name} ${fname})

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

ADD_BINARY(server.cpp sl-server stateline-server)
ADD_BINARY(agent.cpp sl-agent stateline-client)
