# Copyright (c) 2014, NICTA. 
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014 

# Common libraries
ADD_LIBRARY(messages messages.cpp)
ADD_LIBRARY(router router.cpp)
ADD_LIBRARY(socket socket.cpp)
TARGET_LINK_LIBRARIES(router messages socket)

# Server specific libraries
ADD_LIBRARY(serverheartbeat serverheartbeat.cpp)
TARGET_LINK_LIBRARIES(serverheartbeat messages)
ADD_LIBRARY(delegator delegator.cpp)
TARGET_LINK_LIBRARIES(delegator messages router serverheartbeat ${ZMQ_LIBRARY})
ADD_LIBRARY(requester requester.cpp)
TARGET_LINK_LIBRARIES(requester messages router)
ADD_LIBRARY(minion minion.cpp)
TARGET_LINK_LIBRARIES(minion messages router)

# Client specific libraries
ADD_LIBRARY(worker worker.cpp)
TARGET_LINK_LIBRARIES(worker messages router)
ADD_LIBRARY(clientheartbeat clientheartbeat.cpp)
TARGET_LINK_LIBRARIES(clientheartbeat messages)

ADD_TEST(test-message tests/message.cpp messages)
ADD_TEST(test-delegator tests/delegator.cpp delegator)
