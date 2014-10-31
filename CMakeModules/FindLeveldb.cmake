# Copyright (c) 2013, NICTA. 
# This file is licensed under the General Public License version 3 or later.
# See the COPYRIGHT file.

# Find leveldb
#
#  Env variable used as hint for finders:
#  LEVELDB_ROOT_DIR:            Base directory where all LEVELDB components are found
#
# The following are set after configuration is done: 
#  LEVELDB_FOUND
#  LEVELDB_INCLUDE_DIR
#  LEVELDB_LIBRARY

include(FindPackageHandleStandardArgs)

find_path(LEVELDB_INCLUDE_DIR leveldb/db.h
  HINTS
  ${LEVELDB_ROOT_DIR}
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

FIND_LIBRARY(LEVELDB_LIBRARY leveldb
  HINTS
  ${LEVELDB_ROOT_DIR}
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

find_package_handle_standard_args(LEVELDB DEFAULT_MSG
  LEVELDB_INCLUDE_DIR LEVELDB_LIBRARY)
                                

MARK_AS_ADVANCED(LEVELDB_INCLUDE_DIR LEVELDB_LIBRARY)

