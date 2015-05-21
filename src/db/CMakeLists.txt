# Copyright (c) 2014, NICTA. 
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

ADD_LIBRARY(db db.cpp)
TARGET_LINK_LIBRARIES(db ${GLOG_LIBRARY})

#ADD_TEST(test-db tests/db.cpp db ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})
