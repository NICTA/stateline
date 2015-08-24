# Copyright (c) 2013, NICTA. 
# This file is licensed under the General Public License version 3 or later.
# See the COPYRIGHT file.

# Find GLOG
#
#  Env variable used as hint for finders:
#  GLOG_ROOT_DIR:            Base directory where all GLOG components are found
#
# The following are set after configuration is done: 
#  GLOG_FOUND
#  GLOG_INCLUDE_DIR
#  GLOG_LIBRARY

include(FindPackageHandleStandardArgs)

find_path(GLOG_INCLUDE_DIR glog/logging.h
  HINTS ${GLOG_ROOT_DIR}
  PATH_SUFFIXES include
  PATHS
  HINTS ${GLOG_ROOT_DIR}/include
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

FIND_LIBRARY(GLOG_LIBRARY glog
  HINTS
  ${GLOG_ROOT_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  )

find_package_handle_standard_args(GLOG DEFAULT_MSG
                                  GLOG_INCLUDE_DIR GLOG_LIBRARY)
                                

MARK_AS_ADVANCED(GLOG_INCLUDE_DIR GLOG_LIBRARY)

# if(GLOG_FOUND)
    # set(GLOG_INCLUDE_DIR ${GLOG_INCLUDE_DIR})
    # set(GLOG_LIBRARIES ${GLOG_LIBRARY})
# endif()

