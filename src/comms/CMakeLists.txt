# Copyright (c) 2014, NICTA. 
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

# Common libraries
ADD_LIBRARY(comms_messages OBJECT messages.cpp)
ADD_LIBRARY(comms_router OBJECT router.cpp)
ADD_LIBRARY(comms_socket OBJECT socket.cpp)

# Server specific libraries
ADD_LIBRARY(comms_serverheartbeat OBJECT serverheartbeat.cpp)
ADD_LIBRARY(comms_delegator OBJECT delegator.cpp)
ADD_LIBRARY(comms_requester OBJECT requester.cpp)

# Client specific libraries
ADD_LIBRARY(comms_clientheartbeat OBJECT clientheartbeat.cpp)
ADD_LIBRARY(comms_worker OBJECT worker.cpp)
ADD_LIBRARY(comms_minion OBJECT minion.cpp)

ADD_LIBRARY(comms
  $<TARGET_OBJECTS:comms_messages>
  $<TARGET_OBJECTS:comms_router>
  $<TARGET_OBJECTS:comms_socket>
  $<TARGET_OBJECTS:comms_serverheartbeat>
  $<TARGET_OBJECTS:comms_delegator>
  $<TARGET_OBJECTS:comms_requester>
  $<TARGET_OBJECTS:comms_clientheartbeat>
  $<TARGET_OBJECTS:comms_worker>
  $<TARGET_OBJECTS:comms_minion>)

ADD_TEST(test-message tests/message.cpp comms ${ZMQ_LIBRARY})
ADD_TEST(test-socket tests/socket.cpp comms ${ZMQ_LIBRARY})
ADD_TEST(test-router tests/router.cpp comms ${ZMQ_LIBRARY})
ADD_TEST(test-delegator tests/delegator.cpp comms ${ZMQ_LIBRARY})
