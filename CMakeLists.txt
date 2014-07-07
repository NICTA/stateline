# Copyright (c) 2014, NICTA. 
# Affero General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

##############################################################################
# CMAKE SETUP
##############################################################################

CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )

PROJECT ( OBSIDIAN )

# Use folders
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

# Check source and build directories are okay
MESSAGE ( "*** Source directory detected as: ${OBSIDIAN_SOURCE_DIR}" )
MESSAGE ( "*** Build directory detected as: ${OBSIDIAN_BINARY_DIR}" )
SET ( ROOT_SRC_DIR "${OBSIDIAN_SOURCE_DIR}" )
SET ( ROOT_BIN_DIR "${OBSIDIAN_BINARY_DIR}" )

# This is the path to additional CMake modules for finding libs etc.
SET ( CMAKE_MODULE_PATH "${ROOT_SRC_DIR}/CMakeModules/" ${CMAKE_MODULE_PATH})
MESSAGE ( STATUS "Module path ${CMAKE_MODULE_PATH}" )

# Build type defaults
IF ( NOT CMAKE_BUILD_TYPE )
    MESSAGE ( STATUS "Build Type: Unset, assuming Debug" )
    SET (CMAKE_BUILD_TYPE "Debug" )
ELSE ( NOT CMAKE_BUILD_TYPE )
    MESSAGE ( STATUS "Build Type: ${CMAKE_BUILD_TYPE}" )
ENDIF ( NOT CMAKE_BUILD_TYPE )

# INCLUDE ( CheckTypeSize )
# Variables which must be set for compilation
SET ( CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS}" )
#Add some multithreaded build support

# Search variables for prereqs
set(GTEST_SRC_DIR ${PREREQ_DIR}/src/gtest-1.7.0)
set(BOOST_ROOT ${PREREQ_DIR})
set(GLOG_ROOT_DIR ${PREREQ_DIR})
set(PROTOBUF_ROOT ${PREREQ_DIR})
set(ZMQ_ROOT_DIR ${PREREQ_DIR})
set(EIGEN3_ROOT_DIR ${PREREQ_DIR})
set(LEVELDB_ROOT_DIR ${PREREQ_DIR})

##############################################################################
# LOGGING (GLOG)
##############################################################################

FIND_PACKAGE (Glog REQUIRED )
# Logging toggle
OPTION ( NO_LOGGING "Set to true to compile out any Logging statements" OFF)
IF ( NO_LOGGING )
    ADD_DEFINITIONS ( -DNO_LOGGING )
ENDIF ( NO_LOGGING )

# Conditional linking of logging
MACRO ( LINK_LOGGING projectName )
    IF ( NOT NO_LOGGING )
      TARGET_LINK_LIBRARIES (${projectName} ${GLOG_LIBRARY} )
    ENDIF ( NOT NO_LOGGING )
ENDMACRO ( LINK_LOGGING projectName )

##############################################################################
# COMPILATION
##############################################################################

  IF ( CMAKE_COMPILER_IS_GNUCXX )
    SET ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wno-unused-local-typedefs -ggdb -g  -D_GLIBCXX_USE_NANOSLEEP" )
    SET ( CMAKE_CXX_FLAGS_RELWITHDEBINFO "-std=c++11 -Wall -Wno-unused-local-typedefs -ggdb -g -D_GLIBCXX_USE_NANOSLEEP -DNDEBUG -O2" )
    SET ( CMAKE_CXX_FLAGS_RELEASE "-std=c++11 -D_GLIBCXX_USE_NANOSLEEP -DNDEBUG -O2")
  ELSE( CMAKE_COMPILER_IS_GNUCXX)
    SET ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -ggdb -g  -D_GLIBCXX_USE_NANOSLEEP" )
    SET ( CMAKE_CXX_FLAGS_RELWITHDEBINFO "-std=c++11 -Wall -ggdb -g -D_GLIBCXX_USE_NANOSLEEP -DNDEBUG -O2" )
  ENDIF ( CMAKE_COMPILER_IS_GNUCXX )

##############################################################################
# PREREQUISITE LIBRARIES
##############################################################################

# pthreads
IF ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    MESSAGE ( STATUS "Linux detected, telling compiler to link in pthreads..." )
    SET ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread" )
ENDIF ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )

# Boost
SET ( DESIRED_BOOST_VERSION "1_55_0" )
SET ( Boost_ADDITIONAL_VERSIONS "1_55_0" )
SET ( Boost_USE_STATIC_LIBS ON )
SET ( Boost_USE_STATIC_RUNTIME ON )

FIND_PACKAGE ( Boost REQUIRED COMPONENTS date_time
                                         thread
                                         program_options
                                         system
                                         filesystem
                                         timer )

#ZeroMQ
FIND_PACKAGE ( ZMQ REQUIRED )

#Protobuf
#get environment variable for built-in protobuf finder
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${PROTOBUF_ROOT})
FIND_PACKAGE (Protobuf REQUIRED)

#Eigen
FIND_PACKAGE (Eigen3 3.2.0 REQUIRED)

# Python
find_package ( PythonInterp )

# Leveldb
find_package ( Leveldb )

# Google Test
MESSAGE ( STATUS "GTEST directory: ${GTEST_SRC_DIR}" )
SET(GTEST_DIR ${GTEST_SRC_DIR})
SET ( GTEST_INCLUDE_DIR ${GTEST_SRC_DIR}/include )

##############################################################################
# INCLUDE DIRECTORIES
##############################################################################

INCLUDE_DIRECTORIES ("${OBSIDIAN_SOURCE_DIR}/src"
                     "${CMAKE_CURRENT_BINARY_DIR}/src" # for protobuf files
                      ${ZMQ_INCLUDE_DIR}
                      ${GLOG_INCLUDE_DIR}
                      ${Boost_INCLUDE_DIR}
                      ${EIGEN3_INCLUDE_DIR}
                      ${GTEST_DIR}
                      ${GTEST_INCLUDE_DIR} )


##############################################################################
# TESTING
##############################################################################
# setup the unit test target
ADD_CUSTOM_TARGET (run-test-all COMMENT "Unit Tests" )
# adds the given project as a dependency for the "run-tests" target
MACRO ( REGISTER_UNIT_TESTS projectName )
  SET_TARGET_PROPERTIES(${projectName} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OBSIDIAN_BINARY_DIR})
  TARGET_LINK_LIBRARIES (${projectName} gtest test-common)
    ADD_CUSTOM_TARGET ( run-${projectName}
        COMMAND ${projectName} --gtest_output=xml:${OBSIDIAN_BINARY_DIR}/gtest-${projectName}-results.xml
        DEPENDS ${projectName}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running ${projectName}" )
    ADD_DEPENDENCIES ( run-test-all run-${projectName} )
ENDMACRO ( REGISTER_UNIT_TESTS projectName )


##############################################################################
# DOCUMENTATION
##############################################################################
find_package(Doxygen)
if(DOXYGEN_FOUND)
   configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/Doxyfile
                  ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
   add_custom_target(doc
      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)

# # Add "tags" target and make my_project depending on this target.
# set_source_files_properties(tags properties generated true)
# add_custom_target(tags all
#     command ctags -r --c++-kinds=+p --fields=+ias --extra=+q ./src
#     working_directory ${cmake_source_dir})

##############################################################################
# LINK LIBRARIES
##############################################################################


SET (obsidianBaseLibraries settings
                      console
                      io
                      ${ZMQ_LIBRARY}
                      ${GLOG_LIBRARY}
                      ${LEVELDB_LIBRARY}
                      ${Boost_FILESYSTEM_LIBRARY}
                      ${Boost_SYSTEM_LIBRARY}
                      ${Boost_PROGRAM_OPTIONS_LIBRARY})

SET (obsidianAlgoLibraries distrib
                      fwd-gravmag
                      fwd-mt
                      fwd-seismic
                      fwd-contactpoint
                      fwd-thermal
                      world
                      prior
                      chainarray
                      serial
                      db
                      lh
                      distrib)

SET (obsidianCommsLibraries messages
                       serial
                       transport
                       router)

SET (obsidianServerLibraries asyncdelegator
                        serverheartbeat
                        delegator
                        requester
                        input
                        fwd-mt
                        ${obsidianCommsLibraries})

SET (obsidianClientLibraries  clientheartbeat
                         worker
                         minion
                         input
                         ${obsidianCommsLibraries})

##############################################################################
# SUB-PROJECTS
##############################################################################
MACRO ( ADD_SUBPROJECT directory )
  MESSAGE ( STATUS "Adding subproject: ${directory}" )
  ADD_SUBDIRECTORY ( ${directory} )
ENDMACRO ( ADD_SUBPROJECT directory )

ADD_SUBPROJECT(src/test)
ADD_SUBPROJECT(src/app)
ADD_SUBPROJECT(src/serial)
ADD_SUBPROJECT(src/prior)
ADD_SUBPROJECT(src/world)
ADD_SUBPROJECT(src/distrib)
ADD_SUBPROJECT(src/comms)
ADD_SUBPROJECT(src/fwdmodel)
ADD_SUBPROJECT(src/io)
ADD_SUBPROJECT(src/db)
ADD_SUBPROJECT(src/infer)
ADD_SUBPROJECT(src/input)
ADD_SUBPROJECT(src/bin)
ADD_SUBPROJECT(src/likelihood)

