# Copyright (c) 2013, NICTA. 
# This file is licensed under the General Public License version 3 or later.
# See the COPYRIGHT file.

# Find ZMQ
#
#  Env variable used as hint for finders:
#  ZMQ_ROOT_DIR:            Base directory where all ZMQ components are found
#
# The following are set after configuration is done: 
#  ZMQ_FOUND
#  ZMQ_INCLUDE_DIR
#  ZMQ_LIBRARY

include(FindPackageHandleStandardArgs)

find_path(ZMQ_INCLUDE_DIR zmq.h
  HINTS
  ${ZMQ_ROOT_DIR}
  PATH_SUFFIXES include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

FIND_LIBRARY(ZMQ_LIBRARY zmq
  HINTS
  ${ZMQ_ROOT_DIR}
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

find_package_handle_standard_args(ZMQ DEFAULT_MSG
                                  ZMQ_INCLUDE_DIR ZMQ_LIBRARY)
                                

MARK_AS_ADVANCED(ZMQ_INCLUDE_DIR ZMQ_LIBRARY)

