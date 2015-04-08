# Copyright (c) 2014, NICTA. 
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

# Common libraries
ADD_LIBRARY(messages messages.cpp)
ADD_LIBRARY(transport transport.cpp)
ADD_LIBRARY(router router.cpp)
ADD_LIBRARY(socket socket.cpp)
TARGET_LINK_LIBRARIES(router transport messages socket)


# Server specific libraries
ADD_LIBRARY(delegator delegator.cpp)
TARGET_LINK_LIBRARIES(delegator transport messages router)
ADD_LIBRARY(requester requester.cpp)
ADD_LIBRARY(serverheartbeat serverheartbeat.cpp)
TARGET_LINK_LIBRARIES(serverheartbeat messages)

# Client specific libraries
ADD_LIBRARY(worker worker.cpp)
TARGET_LINK_LIBRARIES(worker transport messages router)
ADD_LIBRARY(minion minion.cpp)
ADD_LIBRARY(clientheartbeat clientheartbeat.cpp)
TARGET_LINK_LIBRARIES(clientheartbeat messages)

#ADD_TEST(test-delegator tests/delegator.cpp statelineserver statelineclient)
#ADD_TEST(test-router tests/router.cpp statelineserver statelineclient)
#ADD_TEST(test-transport tests/transport.cpp statelineserver statelineclient)
