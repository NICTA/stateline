# Copyright (c) 2014, NICTA. 
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

# Common libraries
ADD_LIBRARY(messages OBJECT messages.cpp)
ADD_LIBRARY(router OBJECT router.cpp)
ADD_LIBRARY(socket OBJECT socket.cpp)
ADD_LIBRARY(commoncomms $<TARGET_OBJECTS:messages>
                        $<TARGET_OBJECTS:router>
                        $<TARGET_OBJECTS:socket>)

# Server specific libraries
ADD_LIBRARY(serverheartbeat OBJECT serverheartbeat.cpp)
ADD_LIBRARY(delegator OBJECT delegator.cpp)
ADD_LIBRARY(requester OBJECT requester.cpp)
ADD_LIBRARY(servercomms $<TARGET_OBJECTS:serverheartbeat>
                        $<TARGET_OBJECTS:delegator>
                        $<TARGET_OBJECTS:requester>)

# Client specific libraries
ADD_LIBRARY(clientheartbeat OBJECT clientheartbeat.cpp)
ADD_LIBRARY(worker OBJECT worker.cpp)
ADD_LIBRARY(minion OBJECT minion.cpp)
ADD_LIBRARY(clientcomms $<TARGET_OBJECTS:clientheartbeat>
                        $<TARGET_OBJECTS:worker>
                        $<TARGET_OBJECTS:minion>
                        $<TARGET_OBJECTS:messages>
                        $<TARGET_OBJECTS:router>
                        $<TARGET_OBJECTS:socket>)

ADD_TEST(test-message tests/message.cpp commoncomms ${ZMQ_LIBRARY})
ADD_TEST(test-socket tests/socket.cpp commoncomms ${ZMQ_LIBRARY})
ADD_TEST(test-router tests/router.cpp commoncomms ${ZMQ_LIBRARY})
ADD_TEST(test-delegator tests/delegator.cpp commoncomms servercomms ${ZMQ_LIBRARY})
