# Copyright (c) 2014, NICTA. 
# Affero General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

SET ( shardSources
  shard.cpp)

SET ( obsidianSources obsidian.cpp )

IF(CMAKE_BUILD_TYPE MATCHES Release)

  ############################################################################
  # Obsidian Release
  ############################################################################
  ADD_EXECUTABLE (obsidian obsidian.cpp
                           ${OBSIDIAN_SOURCE_DIR}/src/app/blob.cpp
                           ${OBSIDIAN_SOURCE_DIR}/src/comms/blob.cpp
                           ${OBSIDIAN_SOURCE_DIR}/src/db/blob.cpp
                           ${OBSIDIAN_SOURCE_DIR}/src/distrib/blob.cpp
                           ${OBSIDIAN_SOURCE_DIR}/src/fwdmodel/blob.cpp
                           ${OBSIDIAN_SOURCE_DIR}/src/infer/blob.cpp
                           ${OBSIDIAN_SOURCE_DIR}/src/input/blob.cpp
                           ${OBSIDIAN_SOURCE_DIR}/src/io/blob.cpp
                           ${OBSIDIAN_SOURCE_DIR}/src/likelihood/blob.cpp
                           ${OBSIDIAN_SOURCE_DIR}/src/prior/blob.cpp
                           ${OBSIDIAN_SOURCE_DIR}/src/serial/blob.cpp
                           ${OBSIDIAN_SOURCE_DIR}/src/world/blob.cpp)

  ############################################################################
  # Shard Release
  ############################################################################
  ADD_EXECUTABLE (shard shard.cpp
                        ${OBSIDIAN_SOURCE_DIR}/src/app/blob.cpp
                        ${OBSIDIAN_SOURCE_DIR}/src/comms/blob.cpp
                        ${OBSIDIAN_SOURCE_DIR}/src/db/blob.cpp
                        ${OBSIDIAN_SOURCE_DIR}/src/distrib/blob.cpp
                        ${OBSIDIAN_SOURCE_DIR}/src/fwdmodel/blob.cpp
                        ${OBSIDIAN_SOURCE_DIR}/src/infer/blob.cpp
                        ${OBSIDIAN_SOURCE_DIR}/src/input/blob.cpp
                        ${OBSIDIAN_SOURCE_DIR}/src/io/blob.cpp
                        ${OBSIDIAN_SOURCE_DIR}/src/likelihood/blob.cpp
                        ${OBSIDIAN_SOURCE_DIR}/src/prior/blob.cpp
                        ${OBSIDIAN_SOURCE_DIR}/src/serial/blob.cpp
                        ${OBSIDIAN_SOURCE_DIR}/src/world/blob.cpp)

  TARGET_LINK_LIBRARIES(obsidian
                        protobuf
                        ${LEVELDB_LIBRARY}
                        ${PROTOBUF_LIBRARIES}
                        ${ZMQ_LIBRARY}
                        ${GLOG_LIBRARY}
                        ${Boost_FILESYSTEM_LIBRARY}
                        ${Boost_SYSTEM_LIBRARY}
                        ${Boost_PROGRAM_OPTIONS_LIBRARY}
                        ${NLOPT_LIBRARIES}
                        z)

  TARGET_LINK_LIBRARIES(shard
                        protobuf
                        ${LEVELDB_LIBRARY}
                        ${PROTOBUF_LIBRARIES}
                        ${ZMQ_LIBRARY}
                        ${GLOG_LIBRARY}
                        ${Boost_FILESYSTEM_LIBRARY}
                        ${Boost_SYSTEM_LIBRARY}
                        ${Boost_PROGRAM_OPTIONS_LIBRARY}
                        ${NLOPT_LIBRARIES}
                        z)

ELSE(CMAKE_BUILD_TYPE MATCHES Release)

  ############################################################################
  # Obsidian Debug
  ############################################################################
  ADD_EXECUTABLE(obsidian obsidian.cpp ${PROTO_SRC})
  TARGET_LINK_LIBRARIES(obsidian ${obsidianAlgoLibraries}
                                 ${obsidianServerLibraries}
                                 ${obsidianBaseLibraries})

  ############################################################################
  # Shard Debug
  ############################################################################
  ADD_EXECUTABLE(shard shard.cpp)
  TARGET_LINK_LIBRARIES(shard ${obsidianAlgoLibraries}
                              ${obsidianClientLibraries}
                              ${obsidianBaseLibraries})

ENDIF(CMAKE_BUILD_TYPE MATCHES Release)

SET_TARGET_PROPERTIES(shard PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OBSIDIAN_BINARY_DIR})
SET_TARGET_PROPERTIES (obsidian PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OBSIDIAN_BINARY_DIR})

############################################################################
# Prospector
############################################################################
ADD_EXECUTABLE(prospector prospector.cpp)
TARGET_LINK_LIBRARIES(prospector ${obsidianServerLibraries}
                                 ${obsidianAlgoLibraries}
                                 ${obsidianBaseLibraries})
SET_TARGET_PROPERTIES (prospector PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OBSIDIAN_BINARY_DIR})

############################################################################
# Pickaxe
############################################################################
ADD_EXECUTABLE(pickaxe pickaxe.cpp)
TARGET_LINK_LIBRARIES(pickaxe ${obsidianServerLibraries}
                              ${obsidianAlgoLibraries}
                              ${obsidianBaseLibraries})

SET_TARGET_PROPERTIES (pickaxe PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OBSIDIAN_BINARY_DIR})

############################################################################
# Mason
############################################################################
ADD_EXECUTABLE(mason mason.cpp)
TARGET_LINK_LIBRARIES(mason ${obsidianServerLibraries}
                            ${obsidianAlgoLibraries}
                            ${obsidianBaseLibraries})

SET_TARGET_PROPERTIES (mason PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OBSIDIAN_BINARY_DIR})

# Macro for adding demos
FUNCTION (ADD_DEMO demoName)
  ADD_EXECUTABLE (${demoName} ${demoName}.cpp)

  SET_TARGET_PROPERTIES (${demoName} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OBSIDIAN_BINARY_DIR})

  TARGET_LINK_LIBRARIES (${demoName} ${obsidianAlgoLibraries} ${obsidianBaseLibraries} input io serial)
ENDFUNCTION ()

ADD_CUSTOM_TARGET(symlinks ALL)

FUNCTION (SYMLINK_FILE inpath outpath)
  ADD_CUSTOM_COMMAND(TARGET symlinks COMMAND ln -fs ${inpath} ${outpath})
ENDFUNCTION ()

# Create an empty directory to hold visualisation data
FILE (MAKE_DIRECTORY ${OBSIDIAN_BINARY_DIR}/data)

## Copy over the GDF input and configuration files
SYMLINK_FILE (${OBSIDIAN_SOURCE_DIR}/src/bin/input.obsidian ${OBSIDIAN_BINARY_DIR})
