# Copyright (c) 2014, NICTA.
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman
# Date: 2014

ADD_LIBRARY(commoncomms OBJECT messages.cpp router.cpp socket.cpp)
ADD_LIBRARY(servercomms OBJECT serverheartbeat.cpp delegator.cpp requester.cpp)
ADD_LIBRARY(clientcomms OBJECT clientheartbeat.cpp worker.cpp minion.cpp)

#ADD_TEST(test-message tests/message.cpp commoncomms ${ZMQ_LIBRARY})
#ADD_TEST(test-socket tests/socket.cpp commoncomms ${ZMQ_LIBRARY})
#ADD_TEST(test-router tests/router.cpp commoncomms ${ZMQ_LIBRARY})
#ADD_TEST(test-delegator tests/delegator.cpp commoncomms servercomms ${ZMQ_LIBRARY})
