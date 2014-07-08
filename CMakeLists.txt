# Copyright (c) 2014, NICTA. 
# Affero General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

##############################################################################
# CMAKE SETUP
##############################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(STATELINE)

# Use folders
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

# Check source and build directories are okay
MESSAGE("*** Source directory detected as: ${STATELINE_SOURCE_DIR}" )
MESSAGE("*** Build directory detected as: ${STATELINE_BINARY_DIR}" )
MESSAGE("*** Prerequites directory detected as: ${PREREQ_DIR}" )
SET(ROOT_SRC_DIR "${STATELINE_SOURCE_DIR}")
SET(ROOT_BIN_DIR "${STATELINE_BINARY_DIR}")

# This is the path to additional CMake modules for finding libs etc.
SET(CMAKE_MODULE_PATH "${ROOT_SRC_DIR}/CMakeModules/" ${CMAKE_MODULE_PATH})
MESSAGE(STATUS "Module path ${CMAKE_MODULE_PATH}" )

# Build type defaults
IF(NOT CMAKE_BUILD_TYPE)
    MESSAGE(STATUS "Build Type: Unset, assuming Debug")
    SET(CMAKE_BUILD_TYPE "Debug")
ELSE(NOT CMAKE_BUILD_TYPE)
    MESSAGE(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
ENDIF(NOT CMAKE_BUILD_TYPE)

# Variables which must be set for compilation
SET(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS}")

# Search variables for prereqs
SET(GTEST_SRC_DIR ${PREREQ_DIR}/src/gtest-1.7.0)
SET(BOOST_ROOT ${PREREQ_DIR})
SET(GLOG_ROOT_DIR ${PREREQ_DIR})
SET(ZMQ_ROOT_DIR ${PREREQ_DIR})
SET(EIGEN3_ROOT_DIR ${PREREQ_DIR})
SET(LEVELDB_ROOT_DIR ${PREREQ_DIR})

##############################################################################
# LOGGING (GLOG)
##############################################################################

FIND_PACKAGE(Glog REQUIRED)
# Logging toggle
OPTION(NO_LOGGING "Set to true to compile out any Logging statements" OFF)
IF(NO_LOGGING)
    ADD_DEFINITIONS(-DNO_LOGGING)
ENDIF(NO_LOGGING)

# Conditional linking of logging
MACRO(LINK_LOGGING projectName)
    IF(NOT NO_LOGGING)
      TARGET_LINK_LIBRARIES(${projectName} ${GLOG_LIBRARY})
    ENDIF (NOT NO_LOGGING)
ENDMACRO(LINK_LOGGING projectName)

##############################################################################
# COMPILATION
##############################################################################

IF(CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wno-unused-local-typedefs -ggdb -g  -D_GLIBCXX_USE_NANOSLEEP")
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-std=c++11 -Wall -Wno-unused-local-typedefs -ggdb -g -D_GLIBCXX_USE_NANOSLEEP -DNDEBUG -O2")
  SET(CMAKE_CXX_FLAGS_RELEASE "-std=c++11 -D_GLIBCXX_USE_NANOSLEEP -DNDEBUG -O2")
ELSE(CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -ggdb -g  -D_GLIBCXX_USE_NANOSLEEP")
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-std=c++11 -Wall -ggdb -g -D_GLIBCXX_USE_NANOSLEEP -DNDEBUG -O2")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

##############################################################################
# PREREQUISITE LIBRARIES
##############################################################################

# Pthreads
IF(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    MESSAGE (STATUS "Linux detected, telling compiler to link in pthreads...")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

# Boost
SET(DESIRED_BOOST_VERSION "1_55_0")
SET(Boost_ADDITIONAL_VERSIONS "1_55_0")
SET(Boost_USE_STATIC_LIBS ON)
SET(Boost_USE_STATIC_RUNTIME ON)

FIND_PACKAGE(Boost REQUIRED COMPONENTS date_time
                                       thread
                                       program_options
                                       system
                                       filesystem
                                       timer
                                       serialization)

# ZeroMQ
FIND_PACKAGE(ZMQ REQUIRED)

# Eigen
FIND_PACKAGE(Eigen3 3.2.0 REQUIRED)

# Python
FIND_PACKAGE(PythonInterp)

# Leveldb
FIND_PACKAGE(Leveldb)

# Google Test
MESSAGE(STATUS "GTEST directory: ${GTEST_SRC_DIR}")
SET(GTEST_DIR ${GTEST_SRC_DIR})
SET(GTEST_INCLUDE_DIR ${GTEST_SRC_DIR}/include)

##############################################################################
# INCLUDE DIRECTORIES
##############################################################################

INCLUDE_DIRECTORIES("${STATELINE_SOURCE_DIR}/src"
                     ${ZMQ_INCLUDE_DIR}
                     ${GLOG_INCLUDE_DIR}
                     ${Boost_INCLUDE_DIR}
                     ${EIGEN3_INCLUDE_DIR}
                     ${GTEST_DIR}
                     ${GTEST_INCLUDE_DIR})


##############################################################################
# TESTING
##############################################################################
# Setup the unit test target
ADD_CUSTOM_TARGET(run-test-all COMMENT "Unit Tests")

# Adds the given project as a dependency for the "run-tests" target
MACRO(REGISTER_UNIT_TESTS projectName)
  SET_TARGET_PROPERTIES(${projectName} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${STATELINE_BINARY_DIR})
  TARGET_LINK_LIBRARIES (${projectName} gtest test-common)
  ADD_CUSTOM_TARGET(run-${projectName}
    COMMAND ${projectName} --gtest_output=xml:${STATELINE_BINARY_DIR}/gtest-${projectName}-results.xml
    DEPENDS ${projectName}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running ${projectName}")
  ADD_DEPENDENCIES( run-test-all run-${projectName})
ENDMACRO()

MACRO(ADD_TEST testName)
  ADD_EXECUTABLE(${testName} ${testName}.cpp)
  TARGET_LINK_LIBRARIES(${testName} ${StatelineBaseLibraries})
  REGISTER_UNIT_TESTS(${testName})
ENDMACRO()

##############################################################################
# DOCUMENTATION
##############################################################################
FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/Doxyfile
                 ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  ADD_CUSTOM_TARGET(doc
      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen" VERBATIM)
ENDIF(DOXYGEN_FOUND)

# # Add "tags" target and make my_project depending on this target.
# set_source_files_properties(tags properties generated true)
# add_custom_target(tags all
#     command ctags -r --c++-kinds=+p --fields=+ias --extra=+q ./src
#     working_directory ${cmake_source_dir})

##############################################################################
# LINK LIBRARIES
##############################################################################
SET(StatelineBaseLibraries settings
                           console
                           ${GLOG_LIBRARY}
                           ${LEVELDB_LIBRARY}
                           ${Boost_FILESYSTEM_LIBRARY}
                           ${Boost_SYSTEM_LIBRARY}
                           ${Boost_PROGRAM_OPTIONS_LIBRARY})

SET(StatelineCommsLibraries messages
                            serial
                            transport
                            router)

SET(StatelineServerLibraries serverheartbeat
                             delegator
                             requester
                             input
                             ${StatelineCommsLibraries})

SET(StatelineClientLibraries clientheartbeat
                             worker
                             minion
                             input
                             ${StatelineCommsLibraries})

##############################################################################
# SUB-PROJECTS
##############################################################################
MACRO(ADD_SUBPROJECT directory)
  MESSAGE(STATUS "Adding subproject: ${directory}")
  ADD_SUBDIRECTORY(${directory})
ENDMACRO(ADD_SUBPROJECT directory)

ADD_SUBPROJECT(src/app)
ADD_SUBPROJECT(src/distrib)
ADD_SUBPROJECT(src/comms)
ADD_SUBPROJECT(src/db)
ADD_SUBPROJECT(src/infer)
ADD_SUBPROJECT(src/bin)
ADD_SUBPROJECT(src/likelihood)

##############################################################################
# Build final libraries for server and client
##############################################################################
SET(STATELINE_COMMON messages router transport ${ZMQ_LIBRARY})
SET(STATELINE_SERVER delegator requester serverheartbeat chainarray db
                     ${ZMQ_LIBRARY} ${GLOG_LIBRARY} ${LEVELDB_LIBRARY})
SET(STATELINE_CLIENT worker minion clientheartbeat)

ADD_LIBRARY(statelineserver src/app/async.hpp)
TARGET_LINK_LIBRARIES(statelineserver ${STATELINE_COMMON} ${STATELINE_SERVER})
SET_TARGET_PROPERTIES(statelineserver PROPERTIES LINKER_LANGUAGE CXX)

ADD_LIBRARY(statelineclient src/app/worker.hpp)
TARGET_LINK_LIBRARIES(statelineclient ${STATELINE_COMMON} ${STATELINE_CLIENT})
SET_TARGET_PROPERTIES(statelineclient PROPERTIES LINKER_LANGUAGE CXX)
