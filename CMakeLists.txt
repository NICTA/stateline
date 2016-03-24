# Copyright (c) 2014, NICTA.
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014

##############################################################################
# CMAKE SETUP
##############################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(STATELINE LANGUAGES CXX)

# Use folders
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

# This is the path to additional CMake modules for finding libs etc.
SET(CMAKE_MODULE_PATH "${STATELINE_SOURCE_DIR}/CMakeModules")

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
# FIXME FIXME FIXME
SET(GTEST_SRC_DIR /usr/src/gtest)

# Installation
install(DIRECTORY src/ DESTINATION "include/stateline"
  FILES_MATCHING PATTERN "*.hpp")

OPTION(LOCAL_INSTALL "Install to build directory (overrides CMAKE_INSTALL_PREFIX)" ON)
IF(LOCAL_INSTALL)
  SET(CMAKE_INSTALL_PREFIX "${STATELINE_BINARY_DIR}/install")
ENDIF(LOCAL_INSTALL)

##############################################################################
# COMPILATION
##############################################################################

SET(sl_flags "-D_GLIBCXX_USE_NANOSLEEP -std=c++11")
SET(sl_warnings  "-Wall -Wno-unused-local-typedefs -Wextra -pedantic")
SET(sl_debug "-ggdb -g")
SET(sl_release "-DNDEBUG -O2")

IF(CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${sl_flags}")
  SET(CMAKE_CXX_FLAGS_DEBUG "${sl_debug} ${sl_warnings}")
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${sl_debug} ${sl_release}")
  SET(CMAKE_CXX_FLAGS_RELEASE ${sl_release})
ELSE(CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${sl_flags} ${sl_debug} -Wall")
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${sl_debug} ${sl_release} -Wall")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

ADD_DEFINITIONS(-DELPP_THREAD_SAFE)

##############################################################################
# PREREQUISITE LIBRARIES
##############################################################################

# Pthreads
IF(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    MESSAGE (STATUS "Linux detected, telling compiler to link in pthreads...")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

# Boost
IF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # CMake doesn't seem smart enough to detect boost libraries build with Clang
  SET(Boost_COMPILER "-clang-darwin42")
ENDIF()

FIND_PACKAGE(Boost 1.58 REQUIRED COMPONENTS program_options
                                            system
                                            filesystem
                                            regex
                                            coroutine
                                            thread
                                            date_time
                                            context)

# ZeroMQ
FIND_PACKAGE(ZMQ REQUIRED)

# Eigen
FIND_PACKAGE(Eigen3 3.2.0 REQUIRED)

# Google Test
# FIXME FIXME FIXME
#FIND_PACKAGE(GTest REQUIRED)
SET(GTEST_DIR ${GTEST_SRC_DIR})
SET(GTEST_INCLUDE_DIR ${GTEST_SRC_DIR}/include)

##############################################################################
# INCLUDE DIRECTORIES
##############################################################################

INCLUDE_DIRECTORIES("${STATELINE_SOURCE_DIR}/src"
                    SYSTEM "${STATELINE_SOURCE_DIR}/external"
                    SYSTEM "${STATELINE_SOURCE_DIR}/external/cppzmq"
                    SYSTEM "${STATELINE_SOURCE_DIR}/external/json"
                    SYSTEM "${STATELINE_SOURCE_DIR}/external/simple-web-server"
                    SYSTEM "${ZMQ_INCLUDE_DIR}"
                    SYSTEM "${Boost_INCLUDE_DIRS}"
                    SYSTEM "${EIGEN3_INCLUDE_DIR}"
                    SYSTEM "${GTEST_DIR}"
                    SYSTEM "${GTEST_INCLUDE_DIR}")

##############################################################################
# TESTING
##############################################################################
# Setup the unit test target

ADD_LIBRARY(gtest EXCLUDE_FROM_ALL ${GTEST_DIR}/src/gtest-all.cc)
ADD_SUBDIRECTORY(src/test)

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
# SUB-PROJECTS
##############################################################################
MACRO(ADD_SUBPROJECT directory)
  MESSAGE(STATUS "Adding subproject: ${directory}")
  ADD_SUBDIRECTORY(${directory})
ENDMACRO(ADD_SUBPROJECT directory)

ADD_SUBPROJECT(src/app)
ADD_SUBPROJECT(src/comms)
ADD_SUBPROJECT(src/db)
ADD_SUBPROJECT(src/infer)
ADD_SUBPROJECT(src/bin)

##############################################################################
# Build final libraries for server and client
##############################################################################
ADD_LIBRARY(statelineserver STATIC #SHARED
  $<TARGET_OBJECTS:commonwrapper> $<TARGET_OBJECTS:serverwrapper>
  $<TARGET_OBJECTS:commoncomms> $<TARGET_OBJECTS:servercomms>
  $<TARGET_OBJECTS:db> $<TARGET_OBJECTS:mcmc>)
TARGET_LINK_LIBRARIES(statelineserver ${Boost_LIBRARIES} ${ZMQ_LIBRARY})
SET_TARGET_PROPERTIES(statelineserver PROPERTIES LINKER_LANGUAGE CXX)

ADD_LIBRARY(statelineclient STATIC #SHARED
  $<TARGET_OBJECTS:commonwrapper> $<TARGET_OBJECTS:workerwrapper>
  $<TARGET_OBJECTS:commoncomms> $<TARGET_OBJECTS:clientcomms>)
TARGET_LINK_LIBRARIES(statelineclient ${ZMQ_LIBRARY})
SET_TARGET_PROPERTIES(statelineclient PROPERTIES LINKER_LANGUAGE CXX)

##############################################################################
# Move the frontend code to the build folder
##############################################################################
FILE(COPY frontend DESTINATION ${STATELINE_BINARY_DIR})

##############################################################################
# Install binaries
##############################################################################
install (TARGETS statelineserver
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib)
install (TARGETS statelineclient
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib)
